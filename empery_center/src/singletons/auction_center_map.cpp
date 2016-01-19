#include "../precompiled.hpp"
#include "auction_center_map.hpp"
#include "../mmain.hpp"
#include <poseidon/singletons/timer_daemon.hpp>
#include <poseidon/multi_index_map.hpp>
#include <poseidon/job_promise.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include "../auction_center.hpp"
#include "../mysql/auction.hpp"

namespace EmperyCenter {

namespace {
	struct AuctionCenterElement {
		AccountUuid account_uuid;
		std::uint64_t unload_time;

		mutable boost::shared_ptr<const Poseidon::JobPromise> promise;
		mutable boost::shared_ptr<std::deque<boost::shared_ptr<Poseidon::MySql::ObjectBase>>> sink;

		mutable boost::shared_ptr<AuctionCenter> auction_center;
		mutable boost::shared_ptr<Poseidon::TimerItem> timer;

		AuctionCenterElement(AccountUuid account_uuid_, std::uint64_t unload_time_)
			: account_uuid(account_uuid_), unload_time(unload_time_)
		{
		}
	};

	MULTI_INDEX_MAP(AuctionCenterMapContainer, AuctionCenterElement,
		UNIQUE_MEMBER_INDEX(account_uuid)
		MULTI_MEMBER_INDEX(unload_time)
	)

	boost::weak_ptr<AuctionCenterMapContainer> g_auction_center_map;

	void gc_timer_proc(std::uint64_t now){
		PROFILE_ME;
		LOG_EMPERY_CENTER_TRACE("Auction center gc timer: now = ", now);

		const auto auction_center_map = g_auction_center_map.lock();
		if(auction_center_map){
			for(;;){
				const auto it = auction_center_map->begin<1>();
				if(it == auction_center_map->end<1>()){
					break;
				}
				if(now < it->unload_time){
					break;
				}

				// 判定 use_count() 为 0 或 1 的情况。参看 require() 中的注释。
				if((it->promise.use_count() <= 1) && it->auction_center && it->auction_center.unique()){
					LOG_EMPERY_CENTER_INFO("Reclaiming auction center: account_uuid = ", it->account_uuid);
					auction_center_map->erase<1>(it);
				} else {
					auction_center_map->set_key<1, 1>(it, now + 1000);
				}
			}
		}
	}

	MODULE_RAII_PRIORITY(handles, 5000){
		const auto auction_center_map = boost::make_shared<AuctionCenterMapContainer>();
		g_auction_center_map = auction_center_map;
		handles.push(auction_center_map);

		const auto gc_interval = get_config<std::uint64_t>("object_gc_interval", 300000);
		auto timer = Poseidon::TimerDaemon::register_timer(0, gc_interval,
			std::bind(&gc_timer_proc, std::placeholders::_2));
		handles.push(timer);
	}
}

boost::shared_ptr<AuctionCenter> AuctionCenterMap::get(AccountUuid account_uuid){
	PROFILE_ME;

	const auto auction_center_map = g_auction_center_map.lock();
	if(!auction_center_map){
		LOG_EMPERY_CENTER_WARNING("AuctionCenterMap is not loaded.");
		return { };
	}

	auto it = auction_center_map->find<0>(account_uuid);
	if(it == auction_center_map->end<0>()){
		it = auction_center_map->insert<0>(it, AuctionCenterElement(account_uuid, 0));
	}
	if(!it->auction_center){
		LOG_EMPERY_CENTER_INFO("Loading auction center: account_uuid = ", account_uuid);

		if(!it->promise){
			auto sink = boost::make_shared<std::deque<boost::shared_ptr<Poseidon::MySql::ObjectBase>>>();
			std::ostringstream oss;
			oss <<"SELECT * FROM `Center_AuctionTransfer` WHERE `account_uuid` = " <<Poseidon::MySql::UuidFormatter(account_uuid.get());
			auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(sink,
				&MySql::Center_AuctionTransfer::create, "Center_AuctionTransfer", oss.str());
			it->promise = std::move(promise);
			it->sink    = std::move(sink);
		}
		// 复制一个智能指针，并且导致 use_count() 增加。
		// 在 GC 定时器中我们用 use_count() 判定是否有异步操作进行中。
		const auto promise = it->promise;
		Poseidon::JobDispatcher::yield(promise);

		if(it->sink){
			LOG_EMPERY_CENTER_DEBUG("Async MySQL query completed: account_uuid = ", account_uuid, ", rows = ", it->sink->size());

			std::vector<boost::shared_ptr<MySql::Center_AuctionTransfer>> objs;
			objs.reserve(it->sink->size());
			for(auto sit = it->sink->begin(); sit != it->sink->end(); ++sit){
				const auto &base = *sit;
				auto obj = boost::dynamic_pointer_cast<MySql::Center_AuctionTransfer>(base);
				if(!obj){
					LOG_EMPERY_CENTER_ERROR("Unexpected dynamic MySQL object type: type = ", typeid(*base).name());
					DEBUG_THROW(Exception, sslit("Unexpected dynamic MySQL object type"));
				}
				objs.emplace_back(std::move(obj));
			}
			auto auction_center = boost::make_shared<AuctionCenter>(account_uuid, objs);

			const auto auction_center_refresh_interval = get_config<std::uint64_t>("auction_center_refresh_interval", 60000);
			auto timer = Poseidon::TimerDaemon::register_timer(0, auction_center_refresh_interval,
				std::bind([](const boost::weak_ptr<AuctionCenter> &weak){
					PROFILE_ME;
					const auto auction_center = weak.lock();
					if(!auction_center){
						return;
					}
					auction_center->pump_status();
				}, boost::weak_ptr<AuctionCenter>(auction_center))
			);

			it->promise.reset();
			it->sink.reset();
			it->auction_center = std::move(auction_center);
			it->timer = std::move(timer);
		}

		assert(it->auction_center);
	}

	const auto now = Poseidon::get_fast_mono_clock();
	const auto gc_interval = get_config<std::uint64_t>("object_gc_interval", 300000);
	auction_center_map->set_key<0, 1>(it, saturated_add(now, gc_interval));

	return it->auction_center;
}
boost::shared_ptr<AuctionCenter> AuctionCenterMap::require(AccountUuid account_uuid){
	PROFILE_ME;

	auto ret = get(account_uuid);
	if(!ret){
		LOG_EMPERY_CENTER_WARNING("Auction center not found: account_uuid = ", account_uuid);
		DEBUG_THROW(Exception, sslit("Auction center not found"));
	}
	return ret;
}

}
