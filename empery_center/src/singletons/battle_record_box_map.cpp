#include "../precompiled.hpp"
#include "battle_record_box_map.hpp"
#include "../mmain.hpp"
#include <poseidon/singletons/timer_daemon.hpp>
#include <poseidon/multi_index_map.hpp>
#include <poseidon/job_promise.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include "../battle_record_box.hpp"
#include "../mysql/battle_record.hpp"
#include "account_map.hpp"

namespace EmperyCenter {

namespace {
	struct BattleRecordBoxElement {
		AccountUuid account_uuid;
		std::uint64_t unload_time;

		mutable boost::shared_ptr<const Poseidon::JobPromise> promise;
		mutable boost::shared_ptr<std::vector<boost::shared_ptr<MySql::Center_BattleRecord>>> sink;

		mutable boost::shared_ptr<BattleRecordBox> battle_record_box;

		BattleRecordBoxElement(AccountUuid account_uuid_, std::uint64_t unload_time_)
			: account_uuid(account_uuid_), unload_time(unload_time_)
		{
		}
	};

	MULTI_INDEX_MAP(BattleRecordBoxContainer, BattleRecordBoxElement,
		UNIQUE_MEMBER_INDEX(account_uuid)
		MULTI_MEMBER_INDEX(unload_time)
	)

	boost::weak_ptr<BattleRecordBoxContainer> g_battle_record_box_map;

	void gc_timer_proc(std::uint64_t now){
		PROFILE_ME;
		LOG_EMPERY_CENTER_TRACE("Battle record box gc timer: now = ", now);

		const auto battle_record_box_map = g_battle_record_box_map.lock();
		if(!battle_record_box_map){
			return;
		}

		for(;;){
			const auto it = battle_record_box_map->begin<1>();
			if(it == battle_record_box_map->end<1>()){
				break;
			}
			if(now < it->unload_time){
				break;
			}

			// 判定 use_count() 为 0 或 1 的情况。参看 require() 中的注释。
			if((it->promise.use_count() <= 1) && it->battle_record_box && it->battle_record_box.unique()){
				LOG_EMPERY_CENTER_DEBUG("Reclaiming battle record box: account_uuid = ", it->account_uuid);
				battle_record_box_map->erase<1>(it);
			} else {
				battle_record_box_map->set_key<1, 1>(it, now + 1000);
			}
		}
	}

	MODULE_RAII_PRIORITY(handles, 5000){
		const auto battle_record_box_map = boost::make_shared<BattleRecordBoxContainer>();
		g_battle_record_box_map = battle_record_box_map;
		handles.push(battle_record_box_map);

		const auto gc_interval = get_config<std::uint64_t>("object_gc_interval", 300000);
		auto timer = Poseidon::TimerDaemon::register_timer(0, gc_interval,
			std::bind(&gc_timer_proc, std::placeholders::_2));
		handles.push(timer);
	}
}

boost::shared_ptr<BattleRecordBox> BattleRecordBoxMap::get(AccountUuid account_uuid){
	PROFILE_ME;

	const auto battle_record_box_map = g_battle_record_box_map.lock();
	if(!battle_record_box_map){
		LOG_EMPERY_CENTER_WARNING("BattleRecordBoxMap is not loaded.");
		return { };
	}

	const auto account = AccountMap::get(account_uuid);
	if(!account){
		LOG_EMPERY_CENTER_DEBUG("Account not found: account_uuid = ", account_uuid);
		return { };
	}

	auto it = battle_record_box_map->find<0>(account_uuid);
	if(it == battle_record_box_map->end<0>()){
		it = battle_record_box_map->insert<0>(it, BattleRecordBoxElement(account_uuid, 0));
	}
	if(!it->battle_record_box){
		LOG_EMPERY_CENTER_DEBUG("Loading battle record box: account_uuid = ", account_uuid);

		if(!it->promise){
			auto sink = boost::make_shared<std::vector<boost::shared_ptr<MySql::Center_BattleRecord>>>();
			std::ostringstream oss;
			oss <<"SELECT * FROM `Center_BattleRecord` WHERE `first_account_uuid` = " <<Poseidon::MySql::UuidFormatter(account_uuid.get())
			    <<" AND `deleted` = 0";
			auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(
				[sink](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){
					auto obj = boost::make_shared<MySql::Center_BattleRecord>();
					obj->fetch(conn);
					obj->enable_auto_saving();
					sink->emplace_back(std::move(obj));
				}, "Center_BattleRecord", oss.str());
			it->promise = std::move(promise);
			it->sink    = std::move(sink);
		}
		// 复制一个智能指针，并且导致 use_count() 增加。
		// 在 GC 定时器中我们用 use_count() 判定是否有异步操作进行中。
		const auto promise = it->promise;
		Poseidon::JobDispatcher::yield(promise, true);

		if(it->sink){
			LOG_EMPERY_CENTER_DEBUG("Async MySQL query completed: account_uuid = ", account_uuid, ", rows = ", it->sink->size());

			auto battle_record_box = boost::make_shared<BattleRecordBox>(account_uuid, *(it->sink));

			it->promise.reset();
			it->sink.reset();
			it->battle_record_box = std::move(battle_record_box);
		}

		assert(it->battle_record_box);
	}

	const auto now = Poseidon::get_fast_mono_clock();
	const auto gc_interval = get_config<std::uint64_t>("object_gc_interval", 300000);
	battle_record_box_map->set_key<0, 1>(it, saturated_add(now, gc_interval));

	return it->battle_record_box;
}
boost::shared_ptr<BattleRecordBox> BattleRecordBoxMap::require(AccountUuid account_uuid){
	PROFILE_ME;

	auto ret = get(account_uuid);
	if(!ret){
		LOG_EMPERY_CENTER_WARNING("Battle record box not found: account_uuid = ", account_uuid);
		DEBUG_THROW(Exception, sslit("Battle record box not found"));
	}
	return ret;
}

}
