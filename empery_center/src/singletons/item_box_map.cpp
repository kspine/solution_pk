#include "../precompiled.hpp"
#include "item_box_map.hpp"
#include "../mmain.hpp"
#include <poseidon/singletons/timer_daemon.hpp>
#include <poseidon/multi_index_map.hpp>
#include <poseidon/job_promise.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include "../item_box.hpp"
#include "../mysql/item.hpp"

namespace EmperyCenter {

namespace {
	struct ItemBoxElement {
		AccountUuid account_uuid;
		std::uint64_t unload_time;

		mutable boost::shared_ptr<const Poseidon::JobPromise> promise;
		mutable boost::shared_ptr<std::vector<boost::shared_ptr<MySql::Center_Item>>> sink;

		mutable boost::shared_ptr<ItemBox> item_box;
		mutable boost::shared_ptr<Poseidon::TimerItem> timer;

		ItemBoxElement(AccountUuid account_uuid_, std::uint64_t unload_time_)
			: account_uuid(account_uuid_), unload_time(unload_time_)
		{
		}
	};

	MULTI_INDEX_MAP(ItemBoxMapContainer, ItemBoxElement,
		UNIQUE_MEMBER_INDEX(account_uuid)
		MULTI_MEMBER_INDEX(unload_time)
	)

	boost::weak_ptr<ItemBoxMapContainer> g_item_box_map;

	void gc_timer_proc(std::uint64_t now){
		PROFILE_ME;
		LOG_EMPERY_CENTER_TRACE("Item box gc timer: now = ", now);

		const auto item_box_map = g_item_box_map.lock();
		if(item_box_map){
			for(;;){
				const auto it = item_box_map->begin<1>();
				if(it == item_box_map->end<1>()){
					break;
				}
				if(now < it->unload_time){
					break;
				}

				// 判定 use_count() 为 0 或 1 的情况。参看 require() 中的注释。
				if((it->promise.use_count() <= 1) && it->item_box && it->item_box.unique()){
					LOG_EMPERY_CENTER_DEBUG("Reclaiming item box: account_uuid = ", it->account_uuid);
					item_box_map->erase<1>(it);
				} else {
					item_box_map->set_key<1, 1>(it, now + 1000);
				}
			}
		}
	}

	MODULE_RAII_PRIORITY(handles, 5000){
		const auto item_box_map = boost::make_shared<ItemBoxMapContainer>();
		g_item_box_map = item_box_map;
		handles.push(item_box_map);

		const auto gc_interval = get_config<std::uint64_t>("object_gc_interval", 300000);
		auto timer = Poseidon::TimerDaemon::register_timer(0, gc_interval,
			std::bind(&gc_timer_proc, std::placeholders::_2));
		handles.push(timer);
	}
}

boost::shared_ptr<ItemBox> ItemBoxMap::get(AccountUuid account_uuid){
	PROFILE_ME;

	const auto item_box_map = g_item_box_map.lock();
	if(!item_box_map){
		LOG_EMPERY_CENTER_WARNING("ItemBoxMap is not loaded.");
		return { };
	}

	auto it = item_box_map->find<0>(account_uuid);
	if(it == item_box_map->end<0>()){
		it = item_box_map->insert<0>(it, ItemBoxElement(account_uuid, 0));
	}
	if(!it->item_box){
		LOG_EMPERY_CENTER_DEBUG("Loading item box: account_uuid = ", account_uuid);

		if(!it->promise){
			auto sink = boost::make_shared<std::vector<boost::shared_ptr<MySql::Center_Item>>>();
			std::ostringstream oss;
			oss <<"SELECT * FROM `Center_Item` WHERE `account_uuid` = " <<Poseidon::MySql::UuidFormatter(account_uuid.get());
			auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(
				[sink](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){
					auto obj = boost::make_shared<MySql::Center_Item>();
					obj->fetch(conn);
					obj->enable_auto_saving();
					sink->emplace_back(std::move(obj));
				}, "Center_Item", oss.str());
			it->promise = std::move(promise);
			it->sink    = std::move(sink);
		}
		// 复制一个智能指针，并且导致 use_count() 增加。
		// 在 GC 定时器中我们用 use_count() 判定是否有异步操作进行中。
		const auto promise = it->promise;
		Poseidon::JobDispatcher::yield(promise);

		if(it->sink){
			LOG_EMPERY_CENTER_DEBUG("Async MySQL query completed: account_uuid = ", account_uuid, ", rows = ", it->sink->size());

			auto item_box = boost::make_shared<ItemBox>(account_uuid, *(it->sink));
			item_box->check_init_items();

			const auto item_box_refresh_interval = get_config<std::uint64_t>("item_box_refresh_interval", 60000);
			auto timer = Poseidon::TimerDaemon::register_timer(0, item_box_refresh_interval,
				std::bind([](const boost::weak_ptr<ItemBox> &weak){
					PROFILE_ME;
					const auto item_box = weak.lock();
					if(!item_box){
						return;
					}
					item_box->pump_status();
				}, boost::weak_ptr<ItemBox>(item_box))
			);

			it->promise.reset();
			it->sink.reset();
			it->item_box = std::move(item_box);
			it->timer = std::move(timer);
		}

		assert(it->item_box);
	}

	const auto now = Poseidon::get_fast_mono_clock();
	const auto gc_interval = get_config<std::uint64_t>("object_gc_interval", 300000);
	item_box_map->set_key<0, 1>(it, saturated_add(now, gc_interval));

	return it->item_box;
}
boost::shared_ptr<ItemBox> ItemBoxMap::require(AccountUuid account_uuid){
	PROFILE_ME;

	auto ret = get(account_uuid);
	if(!ret){
		LOG_EMPERY_CENTER_WARNING("Item box not found: account_uuid = ", account_uuid);
		DEBUG_THROW(Exception, sslit("Item box not found"));
	}
	return ret;
}

}
