#include "../precompiled.hpp"
#include "friend_record_box_map.hpp"
#include "../mmain.hpp"
#include <poseidon/singletons/timer_daemon.hpp>
#include <poseidon/multi_index_map.hpp>
#include <poseidon/job_promise.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/singletons/event_dispatcher.hpp>
#include "../events/account.hpp"
#include "../friend_record_box.hpp"
#include "../mysql/friend.hpp"
#include "account_map.hpp"

namespace EmperyCenter {

namespace {
	struct FriendRecordBoxElement {
		AccountUuid account_uuid;
		std::uint64_t unload_time;

		mutable boost::shared_ptr<const Poseidon::JobPromise> promise;
		mutable boost::shared_ptr<std::vector<boost::shared_ptr<MySql::Center_FriendRecord>>> sink;

		mutable boost::shared_ptr<FriendRecordBox> friend_record_box;

		FriendRecordBoxElement(AccountUuid account_uuid_, std::uint64_t unload_time_)
			: account_uuid(account_uuid_), unload_time(unload_time_)
		{
		}
	};

	MULTI_INDEX_MAP(FriendRecordBoxContainer, FriendRecordBoxElement,
		UNIQUE_MEMBER_INDEX(account_uuid)
		MULTI_MEMBER_INDEX(unload_time)
	)

	boost::weak_ptr<FriendRecordBoxContainer> g_friend_record_box_map;

	void gc_timer_proc(std::uint64_t now){
		PROFILE_ME;
		LOG_EMPERY_CENTER_TRACE("Friend record box gc timer: now = ", now);

		const auto friend_record_box_map = g_friend_record_box_map.lock();
		if(!friend_record_box_map){
			return;
		}

		for(;;){
			const auto it = friend_record_box_map->begin<1>();
			if(it == friend_record_box_map->end<1>()){
				break;
			}
			if(now < it->unload_time){
				break;
			}

			// 判定 use_count() 为 0 或 1 的情况。参看 require() 中的注释。
			if((it->promise.use_count() | it->friend_record_box.use_count()) != 1){ // (a > 1) || (b > 1) || ((a == 0) && b == 0))
				friend_record_box_map->set_key<1, 1>(it, now + 1000);
			} else {
				LOG_EMPERY_CENTER_DEBUG("Reclaiming Friend record box: account_uuid = ", it->account_uuid);
				friend_record_box_map->erase<1>(it);
			}
		}

		Poseidon::MySqlDaemon::enqueue_for_deleting("Center_FriendRecord",
			"DELETE QUICK `r`.* "
			"  FROM `Center_FriendRecord` AS `r` "
			"  WHERE `r`.`deleted` > 0");
	}

	MODULE_RAII_PRIORITY(handles, 5000){
		const auto friend_record_box_map = boost::make_shared<FriendRecordBoxContainer>();
		g_friend_record_box_map = friend_record_box_map;
		handles.push(friend_record_box_map);

		const auto gc_interval = get_config<std::uint64_t>("object_gc_interval", 300000);

		auto timer = Poseidon::TimerDaemon::register_timer(0, gc_interval,
			std::bind(&gc_timer_proc, std::placeholders::_2));
		handles.push(timer);

		auto listener = Poseidon::EventDispatcher::register_listener<Events::AccountInvalidate>(
			[](const boost::shared_ptr<Events::AccountInvalidate> &event){ FriendRecordBoxMap::unload(event->account_uuid); });
		handles.push(listener);
	}
}

void FriendRecordBoxMap::insert(AccountUuid account_uuid,AccountUuid friend_uuid,std::uint64_t utc_now,int result_type){
	PROFILE_ME;
	if(account_uuid){
		const auto friend_record_box = FriendRecordBoxMap::get(account_uuid);
		if(!friend_record_box){
			LOG_EMPERY_CENTER_DEBUG("Failed to load friend record box: account_uuid = ", account_uuid);
			return;
		}
		friend_record_box->push(utc_now,friend_uuid,result_type);
	}
	if(friend_uuid){
		const auto friend_record_box = FriendRecordBoxMap::get(friend_uuid);
		if(!friend_record_box){
			LOG_EMPERY_CENTER_DEBUG("Failed to load friend record box: friend_uuid = ", friend_uuid);
			return;
		}
		friend_record_box->push(utc_now,account_uuid,-result_type);
	}
}

boost::shared_ptr<FriendRecordBox> FriendRecordBoxMap::get(AccountUuid account_uuid){
	PROFILE_ME;

	const auto friend_record_box_map = g_friend_record_box_map.lock();
	if(!friend_record_box_map){
		LOG_EMPERY_CENTER_WARNING("FriendRecordBoxMap is not loaded.");
		return { };
	}

	auto it = friend_record_box_map->find<0>(account_uuid);
	if(it == friend_record_box_map->end<0>()){
		it = friend_record_box_map->insert<0>(it, FriendRecordBoxElement(account_uuid, 0));
	}
	if(!it->friend_record_box){
		if(!AccountMap::is_holding_controller_token(account_uuid)){
			LOG_EMPERY_CENTER_DEBUG("Failed to acquire controller token: account_uuid = ", account_uuid);
			return { };
		}

		LOG_EMPERY_CENTER_DEBUG("Loading Friend record box: account_uuid = ", account_uuid);

		boost::shared_ptr<const Poseidon::JobPromise> promise_tack;
		do {
			if(!it->promise){
				auto sink = boost::make_shared<std::vector<boost::shared_ptr<MySql::Center_FriendRecord>>>();
				std::ostringstream oss;
				oss <<"SELECT * FROM `Center_FriendRecord` WHERE `account_uuid` = " <<Poseidon::MySql::UuidFormatter(account_uuid.get())
				    <<"  AND `deleted` = 0";
				auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(
					[sink](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){
						auto obj = boost::make_shared<MySql::Center_FriendRecord>();
						obj->fetch(conn);
						obj->enable_auto_saving();
						sink->emplace_back(std::move(obj));
					}, "Center_FriendRecord", oss.str());
				it->promise = std::move(promise);
				it->sink    = std::move(sink);
			}
			// 复制一个智能指针，并且导致 use_count() 增加。
			// 在 GC 定时器中我们用 use_count() 判定是否有异步操作进行中。
			promise_tack = it->promise;
			Poseidon::JobDispatcher::yield(promise_tack, true);
		} while(promise_tack != it->promise);

		if(it->sink){
			friend_record_box_map->set_key<0, 1>(it, 0);
			LOG_EMPERY_CENTER_DEBUG("Async MySQL query completed: account_uuid = ", account_uuid, ", rows = ", it->sink->size());

			auto friend_record_box = boost::make_shared<FriendRecordBox>(account_uuid, *(it->sink));

			it->promise.reset();
			it->sink.reset();
			it->friend_record_box = std::move(friend_record_box);
		}

		assert(it->friend_record_box);
	}

	const auto now = Poseidon::get_fast_mono_clock();
	const auto gc_interval = get_config<std::uint64_t>("object_gc_interval", 300000);
	friend_record_box_map->set_key<0, 1>(it, saturated_add(now, gc_interval));

	return it->friend_record_box;
}
boost::shared_ptr<FriendRecordBox> FriendRecordBoxMap::require(AccountUuid account_uuid){
	PROFILE_ME;

	auto ret = get(account_uuid);
	if(!ret){
		LOG_EMPERY_CENTER_WARNING("Friend record box not found: account_uuid = ", account_uuid);
		DEBUG_THROW(Exception, sslit("Friend record box not found"));
	}
	return ret;
}
void FriendRecordBoxMap::unload(AccountUuid account_uuid){
	PROFILE_ME;

	const auto friend_record_box_map = g_friend_record_box_map.lock();
	if(!friend_record_box_map){
		LOG_EMPERY_CENTER_WARNING("FriendRecordBoxMap is not loaded.");
		return;
	}

	const auto it = friend_record_box_map->find<0>(account_uuid);
	if(it == friend_record_box_map->end<0>()){
		LOG_EMPERY_CENTER_DEBUG("Friend record box not loaded: account_uuid = ", account_uuid);
		return;
	}

	friend_record_box_map->set_key<0, 1>(it, 0);
	it->promise.reset();
	const auto now = Poseidon::get_fast_mono_clock();
	gc_timer_proc(now);
}
}
