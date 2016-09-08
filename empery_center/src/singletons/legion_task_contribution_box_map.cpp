#include "../precompiled.hpp"
#include "legion_task_contribution_box_map.hpp"
#include "../mmain.hpp"
#include <poseidon/singletons/timer_daemon.hpp>
#include <poseidon/multi_index_map.hpp>
#include <poseidon/job_promise.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/singletons/event_dispatcher.hpp>
#include "../events/account.hpp"
#include "../legion_task_contribution_box.hpp"
#include "../mysql/task.hpp"

namespace EmperyCenter {

namespace {
	struct LegionTaskContributionBoxElement {
		LegionUuid legion_uuid;
		std::uint64_t unload_time;

		mutable boost::shared_ptr<const Poseidon::JobPromise> promise;
		mutable boost::shared_ptr<std::vector<boost::shared_ptr<MySql::Center_LegionTaskContribution>>> sink;

		mutable boost::shared_ptr<LegionTaskContributionBox> legion_task_contribution_box;

		LegionTaskContributionBoxElement(LegionUuid legion_uuid_, std::uint64_t unload_time_)
			: legion_uuid(legion_uuid_), unload_time(unload_time_)
		{
		}
	};

	MULTI_INDEX_MAP(LegionTaskContributionBoxContainer, LegionTaskContributionBoxElement,
		UNIQUE_MEMBER_INDEX(legion_uuid)
		MULTI_MEMBER_INDEX(unload_time)
	)

	boost::weak_ptr<LegionTaskContributionBoxContainer> g_legion_task_contribution_box_map;

	void gc_timer_proc(std::uint64_t now){
		PROFILE_ME;
		LOG_EMPERY_CENTER_TRACE("Task legion contribution box gc timer: now = ", now);

		const auto legion_task_contribution_box_map = g_legion_task_contribution_box_map.lock();
		if(!legion_task_contribution_box_map){
			return;
		}

		for(;;){
			const auto it = legion_task_contribution_box_map->begin<1>();
			if(it == legion_task_contribution_box_map->end<1>()){
				break;
			}
			if(now < it->unload_time){
				break;
			}

			// 判定 use_count() 为 0 或 1 的情况。参看 require() 中的注释。
			if((it->promise.use_count() | it->legion_task_contribution_box.use_count()) != 1){ // (a > 1) || (b > 1) || ((a == 0) && b == 0))
				legion_task_contribution_box_map->set_key<1, 1>(it, now + 1000);
			} else {
				LOG_EMPERY_CENTER_DEBUG("Reclaiming task legion contribution box: legion_uuid = ", it->legion_uuid);
				legion_task_contribution_box_map->erase<1>(it);
			}
		}
	}

	MODULE_RAII_PRIORITY(handles, 5000){
		const auto legion_task_contribution_box_map = boost::make_shared<LegionTaskContributionBoxContainer>();
		g_legion_task_contribution_box_map = legion_task_contribution_box_map;
		handles.push(legion_task_contribution_box_map);

		const auto gc_interval = get_config<std::uint64_t>("object_gc_interval", 300000);
		auto timer = Poseidon::TimerDaemon::register_timer(0, gc_interval,
			std::bind(&gc_timer_proc, std::placeholders::_2));
		handles.push(timer);
	}
}

boost::shared_ptr<LegionTaskContributionBox> LegionTaskContributionBoxMap::get(LegionUuid legion_uuid){
	PROFILE_ME;

	const auto legion_task_contribution_box_map = g_legion_task_contribution_box_map.lock();
	if(!legion_task_contribution_box_map){
		LOG_EMPERY_CENTER_WARNING("LegionTaskContributionBoxMap is not loaded.");
		return { };
	}

	auto it = legion_task_contribution_box_map->find<0>(legion_uuid);
	if(it == legion_task_contribution_box_map->end<0>()){
		it = legion_task_contribution_box_map->insert<0>(it, LegionTaskContributionBoxElement(legion_uuid, 0));
	}
	if(!it->legion_task_contribution_box){
		LOG_EMPERY_CENTER_DEBUG("Loading task contribution box: legion_uuid = ", legion_uuid);

		boost::shared_ptr<const Poseidon::JobPromise> promise_tack;
		do {
			if(!it->promise){
				auto sink = boost::make_shared<std::vector<boost::shared_ptr<MySql::Center_LegionTaskContribution>>>();
				std::ostringstream oss;
				oss <<"SELECT * FROM `Center_LegionTaskContribution` WHERE `legion_uuid` = " <<Poseidon::MySql::UuidFormatter(legion_uuid.get());
				auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(
					[sink](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){
						auto obj = boost::make_shared<MySql::Center_LegionTaskContribution>();
						obj->fetch(conn);
						obj->enable_auto_saving();
						sink->emplace_back(std::move(obj));
					}, "Center_LegionTaskContribution", oss.str());
				it->promise = std::move(promise);
				it->sink    = std::move(sink);
			}
			// 复制一个智能指针，并且导致 use_count() 增加。
			// 在 GC 定时器中我们用 use_count() 判定是否有异步操作进行中。
			promise_tack = it->promise;
			Poseidon::JobDispatcher::yield(promise_tack, true);
		} while(promise_tack != it->promise);

		if(it->sink){
			legion_task_contribution_box_map->set_key<0, 1>(it, 0);
			LOG_EMPERY_CENTER_DEBUG("Async MySQL query completed: legion_uuid = ", legion_uuid, ", rows = ", it->sink->size());

			auto legion_task_contribution_box = boost::make_shared<LegionTaskContributionBox>(legion_uuid, *(it->sink));
			it->promise.reset();
			it->sink.reset();
			it->legion_task_contribution_box = std::move(legion_task_contribution_box);
		}

		assert(it->legion_task_contribution_box);
	}

	const auto now = Poseidon::get_fast_mono_clock();
	const auto gc_interval = get_config<std::uint64_t>("object_gc_interval", 300000);
	legion_task_contribution_box_map->set_key<0, 1>(it, saturated_add(now, gc_interval));

	return it->legion_task_contribution_box;
}
boost::shared_ptr<LegionTaskContributionBox> LegionTaskContributionBoxMap::require(LegionUuid legion_uuid){
	PROFILE_ME;

	auto ret = get(legion_uuid);
	if(!ret){
		LOG_EMPERY_CENTER_WARNING("Task legion contribution box not found: legion_uuid = ", legion_uuid);
		DEBUG_THROW(Exception, sslit("Task legion contribution box not found"));
	}
	return ret;
}
void LegionTaskContributionBoxMap::unload(LegionUuid legion_uuid){
	PROFILE_ME;

	const auto legion_task_contribution_box_map = g_legion_task_contribution_box_map.lock();
	if(!legion_task_contribution_box_map){
		LOG_EMPERY_CENTER_WARNING("LegionTaskContributionBoxMap is not loaded.");
		return;
	}

	const auto it = legion_task_contribution_box_map->find<0>(legion_uuid);
	if(it == legion_task_contribution_box_map->end<0>()){
		LOG_EMPERY_CENTER_DEBUG("Task legion contribution box not loaded: legion_uuid = ", legion_uuid);
		return;
	}

	legion_task_contribution_box_map->set_key<0, 1>(it, 0);
	it->promise.reset();
	const auto now = Poseidon::get_fast_mono_clock();
	gc_timer_proc(now);
}

}
