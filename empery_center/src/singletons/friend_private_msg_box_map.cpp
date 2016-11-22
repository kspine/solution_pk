#include "../precompiled.hpp"
#include "friend_private_msg_box_map.hpp"
#include "../mmain.hpp"
#include <poseidon/singletons/timer_daemon.hpp>
#include <poseidon/multi_index_map.hpp>
#include <poseidon/job_promise.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/singletons/event_dispatcher.hpp>
#include "../events/account.hpp"
#include "../friend_private_msg_box.hpp"
#include "../friend_private_msg_data.hpp"
#include "../mysql/friend.hpp"
#include "account_map.hpp"

namespace EmperyCenter {

namespace {
	struct FriendPrivateMsgBoxElement {
		AccountUuid account_uuid;
		std::uint64_t unload_time;

		mutable boost::shared_ptr<const Poseidon::JobPromise> promise;
		mutable boost::shared_ptr<std::vector<boost::shared_ptr<MySql::Center_FriendPrivateMsgRecent>>> sink;

		mutable boost::shared_ptr<FriendPrivateMsgBox> friend_private_msg_box;

		FriendPrivateMsgBoxElement(AccountUuid account_uuid_, std::uint64_t unload_time_)
			: account_uuid(account_uuid_), unload_time(unload_time_)
		{
		}
	};

	MULTI_INDEX_MAP(FriendPrivateMsgBoxContainer, FriendPrivateMsgBoxElement,
		UNIQUE_MEMBER_INDEX(account_uuid)
		MULTI_MEMBER_INDEX(unload_time)
	)

	boost::weak_ptr<FriendPrivateMsgBoxContainer> g_friend_private_msg_box_map;

	void gc_timer_proc(std::uint64_t now){
		PROFILE_ME;
		LOG_EMPERY_CENTER_TRACE("Friend private msg box gc timer: now = ", now);

		const auto friend_private_msg_box_map = g_friend_private_msg_box_map.lock();
		if(!friend_private_msg_box_map){
			return;
		}

		for(;;){
			const auto it = friend_private_msg_box_map->begin<1>();
			if(it == friend_private_msg_box_map->end<1>()){
				break;
			}
			if(now < it->unload_time){
				break;
			}

			// 判定 use_count() 为 0 或 1 的情况。参看 require() 中的注释。
			if((it->promise.use_count() | it->friend_private_msg_box.use_count()) != 1){ // (a > 1) || (b > 1) || ((a == 0) && b == 0))
				friend_private_msg_box_map->set_key<1, 1>(it, now + 1000);
			} else {
				LOG_EMPERY_CENTER_DEBUG("Reclaiming Friend private msg box: account_uuid = ", it->account_uuid);
				friend_private_msg_box_map->erase<1>(it);
			}
		}
		
		Poseidon::MySqlDaemon::enqueue_for_deleting("Center_FriendPrivateMsgRecent",
			"DELETE QUICK `r`.* "
			"  FROM `Center_FriendPrivateMsgRecent` AS `r` "
			"  WHERE `r`.`deleted` > 0");
	}
	
	
	struct FriendPrivateMsgDataElement {
		std::pair<FriendPrivateMsgUuid, LanguageId> pkey;
		std::uint64_t unload_time;

		mutable boost::shared_ptr<const Poseidon::JobPromise> promise;
		mutable boost::shared_ptr<boost::container::flat_map<LanguageId, boost::shared_ptr<MySql::Center_FriendPrivateMsgData>>> sink;

		mutable boost::shared_ptr<MySql::Center_FriendPrivateMsgData> msg_data_obj;
		mutable boost::shared_ptr<FriendPrivateMsgData> msg_data;

		FriendPrivateMsgDataElement(FriendPrivateMsgUuid msg_uuid_, LanguageId language_id_, std::uint64_t unload_time_)
			: pkey(msg_uuid_, language_id_), unload_time(unload_time_)
		{
		}
	};

	MULTI_INDEX_MAP(FriendPrivateMsgDataContainer, FriendPrivateMsgDataElement,
		UNIQUE_MEMBER_INDEX(pkey)
		MULTI_MEMBER_INDEX(unload_time)
	)

	boost::weak_ptr<FriendPrivateMsgDataContainer> g_friend_private_msg_data_map;
	
	void data_gc_timer_proc(std::uint64_t now){
		PROFILE_ME;
		LOG_EMPERY_CENTER_TRACE("friend private msg data gc timer: now = ", now);

		const auto friend_private_msg_data_map = g_friend_private_msg_data_map.lock();
		if(!friend_private_msg_data_map){
			return;
		}

		for(;;){
			const auto it = friend_private_msg_data_map->begin<1>();
			if(it == friend_private_msg_data_map->end<1>()){
				break;
			}
			if(now < it->unload_time){
				break;
			}

			// 判定 use_count() 为 0 或 1 的情况。参看 require() 中的注释。
			if((it->promise.use_count() | it->msg_data.use_count()) != 1){ // (a > 1) || (b > 1) || ((a == 0) && b == 0))
				friend_private_msg_data_map->set_key<1, 1>(it, now + 1000);
			} else {
				LOG_EMPERY_CENTER_DEBUG("Reclaiming friend msg data: msg_uuid = ", it->pkey.first);
				friend_private_msg_data_map->erase<1>(it);
			}
		}
	}
	
	MODULE_RAII_PRIORITY(handles, 5000){
		const auto friend_private_msg_box_map = boost::make_shared<FriendPrivateMsgBoxContainer>();
		g_friend_private_msg_box_map = friend_private_msg_box_map;
		handles.push(friend_private_msg_box_map);
		
		const auto friend_private_msg_data_map = boost::make_shared<FriendPrivateMsgDataContainer>();
		g_friend_private_msg_data_map = friend_private_msg_data_map;
		handles.push(friend_private_msg_data_map);

		const auto gc_interval = get_config<std::uint64_t>("object_gc_interval", 300000);

		auto timer = Poseidon::TimerDaemon::register_timer(0, gc_interval,
			std::bind(&gc_timer_proc, std::placeholders::_2));
		handles.push(timer);
		
		timer = Poseidon::TimerDaemon::register_timer(0, gc_interval,
			std::bind(&data_gc_timer_proc, std::placeholders::_2));
		handles.push(timer);

		auto listener = Poseidon::EventDispatcher::register_listener<Events::AccountInvalidate>(
			[](const boost::shared_ptr<Events::AccountInvalidate> &event){ FriendPrivateMsgBoxMap::unload(event->account_uuid); });
		handles.push(listener);
		
	}
}

void FriendPrivateMsgBoxMap::insert(AccountUuid account_uuid,AccountUuid friend_uuid,std::uint64_t utc_now,FriendPrivateMsgUuid msg_uuid,bool sender,bool read,bool deleted){
	PROFILE_ME;

	if(account_uuid){
		const auto friend_private_msg_box = FriendPrivateMsgBoxMap::get(account_uuid);
		if(!friend_private_msg_box){
			LOG_EMPERY_CENTER_DEBUG("Failed to load friend private msg box: account_uuid = ", account_uuid);
			return;
		}
		friend_private_msg_box->push(utc_now,friend_uuid,msg_uuid,sender,read,deleted);
	}
}

boost::shared_ptr<FriendPrivateMsgBox> FriendPrivateMsgBoxMap::get(AccountUuid account_uuid){
	PROFILE_ME;

	const auto friend_private_msg_box_map = g_friend_private_msg_box_map.lock();
	if(!friend_private_msg_box_map){
		LOG_EMPERY_CENTER_WARNING("FriendPrivateMsgBoxMap is not loaded.");
		return { };
	}

	auto it = friend_private_msg_box_map->find<0>(account_uuid);
	if(it == friend_private_msg_box_map->end<0>()){
		it = friend_private_msg_box_map->insert<0>(it, FriendPrivateMsgBoxElement(account_uuid, 0));
	}
	if(!it->friend_private_msg_box){
		if(!AccountMap::is_holding_controller_token(account_uuid)){
			LOG_EMPERY_CENTER_DEBUG("Failed to acquire controller token: account_uuid = ", account_uuid);
			return { };
		}

		LOG_EMPERY_CENTER_DEBUG("Loading Friend private msg box: account_uuid = ", account_uuid);

		boost::shared_ptr<const Poseidon::JobPromise> promise_tack;
		do {
			if(!it->promise){
				auto sink = boost::make_shared<std::vector<boost::shared_ptr<MySql::Center_FriendPrivateMsgRecent>>>();
				std::ostringstream oss;
				oss <<"SELECT * FROM `Center_FriendPrivateMsgRecent` WHERE `account_uuid` = " <<Poseidon::MySql::UuidFormatter(account_uuid.get())
				    <<"  AND `deleted` = 0";
				auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(
					[sink](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){
						auto obj = boost::make_shared<MySql::Center_FriendPrivateMsgRecent>();
						obj->fetch(conn);
						obj->enable_auto_saving();
						sink->emplace_back(std::move(obj));
					}, "Center_FriendPrivateMsgRecent", oss.str());
				it->promise = std::move(promise);
				it->sink    = std::move(sink);
			}
			// 复制一个智能指针，并且导致 use_count() 增加。
			// 在 GC 定时器中我们用 use_count() 判定是否有异步操作进行中。
			promise_tack = it->promise;
			Poseidon::JobDispatcher::yield(promise_tack, true);
		} while(promise_tack != it->promise);

		if(it->sink){
			friend_private_msg_box_map->set_key<0, 1>(it, 0);
			LOG_EMPERY_CENTER_DEBUG("Async MySQL query completed: account_uuid = ", account_uuid, ", rows = ", it->sink->size());

			auto friend_private_msg_box = boost::make_shared<FriendPrivateMsgBox>(account_uuid, *(it->sink));

			it->promise.reset();
			it->sink.reset();
			it->friend_private_msg_box = std::move(friend_private_msg_box);
		}

		assert(it->friend_private_msg_box);
	}

	const auto now = Poseidon::get_fast_mono_clock();
	const auto gc_interval = get_config<std::uint64_t>("object_gc_interval", 300000);
	friend_private_msg_box_map->set_key<0, 1>(it, saturated_add(now, gc_interval));

	return it->friend_private_msg_box;
}
boost::shared_ptr<FriendPrivateMsgBox> FriendPrivateMsgBoxMap::require(AccountUuid account_uuid){
	PROFILE_ME;

	auto ret = get(account_uuid);
	if(!ret){
		LOG_EMPERY_CENTER_WARNING("Friend private msg box not found: account_uuid = ", account_uuid);
		DEBUG_THROW(Exception, sslit("Friend private msg box not found"));
	}
	return ret;
}
void FriendPrivateMsgBoxMap::unload(AccountUuid account_uuid){
	PROFILE_ME;

	const auto friend_private_msg_box_map = g_friend_private_msg_box_map.lock();
	if(!friend_private_msg_box_map){
		LOG_EMPERY_CENTER_WARNING("FriendPrivateMsgBoxMap is not loaded.");
		return;
	}

	const auto it = friend_private_msg_box_map->find<0>(account_uuid);
	if(it == friend_private_msg_box_map->end<0>()){
		LOG_EMPERY_CENTER_DEBUG("Friend private msg box not loaded: account_uuid = ", account_uuid);
		return;
	}

	friend_private_msg_box_map->set_key<0, 1>(it, 0);
	it->promise.reset();
	const auto now = Poseidon::get_fast_mono_clock();
	gc_timer_proc(now);
}

boost::shared_ptr<FriendPrivateMsgData> FriendPrivateMsgBoxMap::get_msg_data(FriendPrivateMsgUuid msg_uuid, LanguageId language_id){
	PROFILE_ME;

	const auto friend_private_msg_data_map = g_friend_private_msg_data_map.lock();
	if(!friend_private_msg_data_map){
		LOG_EMPERY_CENTER_WARNING("FriendPrivateMsgData is not loaded.");
		return { };
	}

	auto it = friend_private_msg_data_map->find<0>(std::make_pair(msg_uuid, language_id));
	if(it == friend_private_msg_data_map->end<0>()){
		it = friend_private_msg_data_map->insert<0>(it, FriendPrivateMsgDataElement(msg_uuid, language_id, 0));
	}
	if(!it->msg_data){
		LOG_EMPERY_CENTER_DEBUG("Loading msg data: msg_uuid = ", msg_uuid, ", language_id = ", language_id);

		if(!it->promise){
			auto sink = boost::make_shared<boost::container::flat_map<LanguageId, boost::shared_ptr<MySql::Center_FriendPrivateMsgData>>>();
			std::ostringstream oss;
			oss <<"SELECT * FROM `Center_FriendPrivateMsgData` WHERE `msg_uuid` = " <<Poseidon::MySql::UuidFormatter(msg_uuid.get());
			auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(
				[sink](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){
					auto obj = boost::make_shared<MySql::Center_FriendPrivateMsgData>();
					obj->fetch(conn);
					obj->enable_auto_saving();
					const auto language_id = LanguageId(obj->get_language_id());
					sink->emplace(language_id, std::move(obj));
				}, "Center_FriendPrivateMsgData", oss.str());
			it->promise = std::move(promise);
			it->sink    = std::move(sink);
		}
		// 复制一个智能指针，并且导致 use_count() 增加。
		// 在 GC 定时器中我们用 use_count() 判定是否有异步操作进行中。
		const auto promise = it->promise;
		Poseidon::JobDispatcher::yield(promise, true);

		if(it->sink){
			friend_private_msg_data_map->set_key<0, 1>(it, 0);
			LOG_EMPERY_CENTER_DEBUG("Async MySQL query completed: msg_uuid = ", msg_uuid, ", rows = ", it->sink->size());

			boost::shared_ptr<MySql::Center_FriendPrivateMsgData> obj;
			decltype(it->sink->cbegin()) sit;
			if((sit = it->sink->find(language_id)) != it->sink->end()){
				obj = sit->second;
			} else if((sit = it->sink->find(LanguageId(0))) != it->sink->end()){
				obj = sit->second;
			} else if((sit = it->sink->begin()) != it->sink->end()){
				obj = sit->second;
			}
			if(!obj){
				LOG_EMPERY_CENTER_WARNING("Mail data not found: msg_uuid = ", msg_uuid, ", rows = ", it->sink->size());
				DEBUG_THROW(Exception, sslit("Mail data not found"));
			}

			auto msg_data = boost::make_shared<FriendPrivateMsgData>(std::move(obj));

			it->promise.reset();
			it->sink.reset();
			it->msg_data = std::move(msg_data);
		}

		assert(it->msg_data);
	}

	const auto now = Poseidon::get_fast_mono_clock();
	const auto gc_interval = get_config<std::uint64_t>("object_gc_interval", 300000);
	friend_private_msg_data_map->set_key<0, 1>(it, saturated_add(now, gc_interval));

	return it->msg_data;
}

boost::shared_ptr<FriendPrivateMsgData> FriendPrivateMsgBoxMap::require_msg_data(FriendPrivateMsgUuid msg_uuid, LanguageId language_id){
	PROFILE_ME;

	auto ret = get_msg_data(msg_uuid, language_id);
	if(!ret){
		LOG_EMPERY_CENTER_WARNING("msg data not found: msg_uuid = ", msg_uuid, ", language_id = ", language_id);
		DEBUG_THROW(Exception, sslit("msg data not found"));
	}
	return ret;
}
void FriendPrivateMsgBoxMap::insert_private_msg_data(boost::shared_ptr<FriendPrivateMsgData> private_msg_data){
	PROFILE_ME;

	const auto friend_private_msg_data_map = g_friend_private_msg_data_map.lock();
	if(!friend_private_msg_data_map){
		LOG_EMPERY_CENTER_WARNING("msg data map is not loaded.");
		DEBUG_THROW(Exception, sslit("msg data map is not loaded"));
	}

	const auto msg_uuid = private_msg_data->get_msg_uuid();
	const auto language_id = private_msg_data->get_language_id();

	const auto now = Poseidon::get_fast_mono_clock();
	const auto gc_interval = get_config<std::uint64_t>("object_gc_interval", 300000);

	const auto result = friend_private_msg_data_map->insert(FriendPrivateMsgDataElement(msg_uuid, language_id, saturated_add(now, gc_interval)));
	if(!result.second){
		LOG_EMPERY_CENTER_WARNING("msg data already exists: msg_uuid = ", msg_uuid, ", language_id = ", language_id);
		DEBUG_THROW(Exception, sslit("msg data already exists"));
	}

	result.first->msg_data = std::move(private_msg_data);
}

}
