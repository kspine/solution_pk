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
#include "account_map.hpp"

namespace EmperyCenter {

namespace {
	struct AuctionCenterElement {
		AccountUuid account_uuid;
		std::uint64_t unload_time;

		mutable boost::shared_ptr<const Poseidon::JobPromise> promise;
		mutable boost::shared_ptr<std::vector<boost::shared_ptr<MySql::Center_AuctionTransfer>>> sink;

		mutable boost::shared_ptr<AuctionCenter> auction_center;
		mutable boost::shared_ptr<Poseidon::TimerItem> timer;

		AuctionCenterElement(AccountUuid account_uuid_, std::uint64_t unload_time_)
			: account_uuid(account_uuid_), unload_time(unload_time_)
		{
		}
	};

	MULTI_INDEX_MAP(AuctionCenterContainer, AuctionCenterElement,
		UNIQUE_MEMBER_INDEX(account_uuid)
		MULTI_MEMBER_INDEX(unload_time)
	)

	boost::weak_ptr<AuctionCenterContainer> g_auction_center_map;

	void gc_timer_proc(std::uint64_t now){
		PROFILE_ME;
		LOG_EMPERY_CENTER_TRACE("Auction center gc timer: now = ", now);

		const auto auction_center_map = g_auction_center_map.lock();
		if(!auction_center_map){
			return;
		}

		for(;;){
			const auto it = auction_center_map->begin<1>();
			if(it == auction_center_map->end<1>()){
				break;
			}
			if(now < it->unload_time){
				break;
			}

			// 判定 use_count() 为 0 或 1 的情况。参看 require() 中的注释。
			if((it->promise.use_count() | it->auction_center.use_count()) != 1){ // (a > 1) || (b > 1) || ((a == 0) && b == 0))
				auction_center_map->set_key<1, 1>(it, now + 1000);
			} else {
				LOG_EMPERY_CENTER_DEBUG("Reclaiming auction center: account_uuid = ", it->account_uuid);
				auction_center_map->erase<1>(it);
			}
		}
	}

	MODULE_RAII_PRIORITY(handles, 5000){
		const auto auction_center_map = boost::make_shared<AuctionCenterContainer>();
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

	if(!AccountMap::is_holding_controller_token(account_uuid)){
		LOG_EMPERY_CENTER_DEBUG("Failed to acquire controller token: account_uuid = ", account_uuid);
		return { };
	}

	auto it = auction_center_map->find<0>(account_uuid);
	if(it == auction_center_map->end<0>()){
		it = auction_center_map->insert<0>(it, AuctionCenterElement(account_uuid, 0));
	}
	if(!it->auction_center){
		LOG_EMPERY_CENTER_DEBUG("Loading auction center: account_uuid = ", account_uuid);

		boost::shared_ptr<const Poseidon::JobPromise> promise_tack;
		do {
			if(!it->promise){
				auto sink = boost::make_shared<std::vector<boost::shared_ptr<MySql::Center_AuctionTransfer>>>();
				std::ostringstream oss;
				oss <<"SELECT * FROM `Center_AuctionTransfer` WHERE `account_uuid` = " <<Poseidon::MySql::UuidFormatter(account_uuid.get());
				auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(
					[sink](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){
						auto obj = boost::make_shared<MySql::Center_AuctionTransfer>();
						obj->fetch(conn);
						obj->enable_auto_saving();
						sink->emplace_back(std::move(obj));
					}, "Center_AuctionTransfer", oss.str());
				it->promise = std::move(promise);
				it->sink    = std::move(sink);
			}
			// 复制一个智能指针，并且导致 use_count() 增加。
			// 在 GC 定时器中我们用 use_count() 判定是否有异步操作进行中。
			promise_tack = it->promise;
			Poseidon::JobDispatcher::yield(promise_tack, true);
		} while(promise_tack != it->promise);

		if(it->sink){
			auction_center_map->set_key<0, 1>(it, 0);
			LOG_EMPERY_CENTER_DEBUG("Async MySQL query completed: account_uuid = ", account_uuid, ", rows = ", it->sink->size());

			auto auction_center = boost::make_shared<AuctionCenter>(account_uuid, *(it->sink));

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
void AuctionCenterMap::unload(AccountUuid account_uuid){
	PROFILE_ME;

	const auto auction_center_map = g_auction_center_map.lock();
	if(!auction_center_map){
		LOG_EMPERY_CENTER_WARNING("AuctionCenterMap is not loaded.");
		return;
	}

	const auto it = auction_center_map->find<0>(account_uuid);
	if(it == auction_center_map->end<0>()){
		LOG_EMPERY_CENTER_DEBUG("Auction center not loaded: account_uuid = ", account_uuid);
		return;
	}

	auction_center_map->set_key<0, 1>(it, 0);
	it->promise.reset();
	const auto now = Poseidon::get_fast_mono_clock();
	gc_timer_proc(now);
}

}
