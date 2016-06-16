#include "../precompiled.hpp"
#include "task_box_map.hpp"
#include "../mmain.hpp"
#include <poseidon/singletons/timer_daemon.hpp>
#include <poseidon/multi_index_map.hpp>
#include <poseidon/job_promise.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include "../task_box.hpp"
#include "../mysql/task.hpp"
#include "account_map.hpp"

namespace EmperyCenter {

namespace {
	struct TaskBoxElement {
		AccountUuid account_uuid;
		std::uint64_t unload_time;

		mutable boost::shared_ptr<const Poseidon::JobPromise> promise;
		mutable boost::shared_ptr<std::vector<boost::shared_ptr<MySql::Center_Task>>> sink;

		mutable boost::shared_ptr<TaskBox> task_box;
		mutable boost::shared_ptr<Poseidon::TimerItem> timer;

		TaskBoxElement(AccountUuid account_uuid_, std::uint64_t unload_time_)
			: account_uuid(account_uuid_), unload_time(unload_time_)
		{
		}
	};

	MULTI_INDEX_MAP(TaskBoxContainer, TaskBoxElement,
		UNIQUE_MEMBER_INDEX(account_uuid)
		MULTI_MEMBER_INDEX(unload_time)
	)

	boost::weak_ptr<TaskBoxContainer> g_task_box_map;

	void gc_timer_proc(std::uint64_t now){
		PROFILE_ME;
		LOG_EMPERY_CENTER_TRACE("Task box gc timer: now = ", now);

		const auto task_box_map = g_task_box_map.lock();
		if(!task_box_map){
			return;
		}

		for(;;){
			const auto it = task_box_map->begin<1>();
			if(it == task_box_map->end<1>()){
				break;
			}
			if(now < it->unload_time){
				break;
			}

			// 判定 use_count() 为 0 或 1 的情况。参看 require() 中的注释。
			if((it->promise.use_count() | it->task_box.use_count()) != 1){ // (a > 1) || (b > 1) || ((a == 0) && b == 0))
				task_box_map->set_key<1, 1>(it, now + 1000);
			} else {
				LOG_EMPERY_CENTER_DEBUG("Reclaiming task box: account_uuid = ", it->account_uuid);
				task_box_map->erase<1>(it);
			}
		}
	}

	MODULE_RAII_PRIORITY(handles, 5000){
		const auto task_box_map = boost::make_shared<TaskBoxContainer>();
		g_task_box_map = task_box_map;
		handles.push(task_box_map);

		const auto gc_interval = get_config<std::uint64_t>("object_gc_interval", 300000);
		auto timer = Poseidon::TimerDaemon::register_timer(0, gc_interval,
			std::bind(&gc_timer_proc, std::placeholders::_2));
		handles.push(timer);
	}
}

boost::shared_ptr<TaskBox> TaskBoxMap::get(AccountUuid account_uuid){
	PROFILE_ME;

	const auto task_box_map = g_task_box_map.lock();
	if(!task_box_map){
		LOG_EMPERY_CENTER_WARNING("TaskBoxMap is not loaded.");
		return { };
	}

	const auto account = AccountMap::get(account_uuid);
	if(!account){
		LOG_EMPERY_CENTER_DEBUG("Account not found: account_uuid = ", account_uuid);
		return { };
	}

	auto it = task_box_map->find<0>(account_uuid);
	if(it == task_box_map->end<0>()){
		it = task_box_map->insert<0>(it, TaskBoxElement(account_uuid, 0));
	}
	if(!it->task_box){
		LOG_EMPERY_CENTER_DEBUG("Loading task box: account_uuid = ", account_uuid);

		boost::shared_ptr<const Poseidon::JobPromise> promise_tack;
		do {
			if(!it->promise){
				auto sink = boost::make_shared<std::vector<boost::shared_ptr<MySql::Center_Task>>>();
				std::ostringstream oss;
				oss <<"SELECT * FROM `Center_Task` WHERE `account_uuid` = " <<Poseidon::MySql::UuidFormatter(account_uuid.get());
				auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(
					[sink](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){
						auto obj = boost::make_shared<MySql::Center_Task>();
						obj->fetch(conn);
						obj->enable_auto_saving();
						sink->emplace_back(std::move(obj));
					}, "Center_Task", oss.str());
				it->promise = std::move(promise);
				it->sink    = std::move(sink);
			}
			// 复制一个智能指针，并且导致 use_count() 增加。
			// 在 GC 定时器中我们用 use_count() 判定是否有异步操作进行中。
			promise_tack = it->promise;
			Poseidon::JobDispatcher::yield(promise_tack, true);
		} while(promise_tack != it->promise);

		if(it->sink){
			task_box_map->set_key<0, 1>(it, 0);
			LOG_EMPERY_CENTER_DEBUG("Async MySQL query completed: account_uuid = ", account_uuid, ", rows = ", it->sink->size());

			auto task_box = boost::make_shared<TaskBox>(account_uuid, *(it->sink));
			task_box->check_primary_tasks();

			const auto task_box_refresh_interval = get_config<std::uint64_t>("task_box_refresh_interval", 60000);
			auto timer = Poseidon::TimerDaemon::register_timer(0, task_box_refresh_interval,
				std::bind([](const boost::weak_ptr<TaskBox> &weak){
					PROFILE_ME;
					const auto task_box = weak.lock();
					if(!task_box){
						return;
					}
					task_box->pump_status();
				}, boost::weak_ptr<TaskBox>(task_box))
			);

			it->promise.reset();
			it->sink.reset();
			it->task_box = std::move(task_box);
			it->timer = std::move(timer);
		}

		assert(it->task_box);
	}

	const auto now = Poseidon::get_fast_mono_clock();
	const auto gc_interval = get_config<std::uint64_t>("object_gc_interval", 300000);
	task_box_map->set_key<0, 1>(it, saturated_add(now, gc_interval));

	return it->task_box;
}
boost::shared_ptr<TaskBox> TaskBoxMap::require(AccountUuid account_uuid){
	PROFILE_ME;

	auto ret = get(account_uuid);
	if(!ret){
		LOG_EMPERY_CENTER_WARNING("Task box not found: account_uuid = ", account_uuid);
		DEBUG_THROW(Exception, sslit("Task box not found"));
	}
	return ret;
}

}
