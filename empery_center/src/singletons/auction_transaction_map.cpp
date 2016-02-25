#include "../precompiled.hpp"
#include "auction_transaction_map.hpp"
#include "../mmain.hpp"
#include <poseidon/multi_index_map.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/singletons/timer_daemon.hpp>
#include "../auction_transaction.hpp"
#include "../mysql/auction.hpp"
#include "../string_utilities.hpp"

namespace EmperyCenter {

namespace {
	struct AuctionTransactionElement {
		boost::shared_ptr<AuctionTransaction> auction_transaction;

		std::size_t serial_hash;
		std::pair<AccountUuid, std::uint64_t> account_uuid_created_time;
		std::uint64_t expiry_time;

		explicit AuctionTransactionElement(boost::shared_ptr<AuctionTransaction> auction_transaction_)
			: auction_transaction(std::move(auction_transaction_))
			, serial_hash(hash_string_nocase(auction_transaction->get_serial()))
			, account_uuid_created_time(auction_transaction->get_account_uuid(), auction_transaction->get_created_time())
			, expiry_time(auction_transaction->get_expiry_time())
		{
		}
	};

	MULTI_INDEX_MAP(AuctionTransactionMapContainer, AuctionTransactionElement,
		MULTI_MEMBER_INDEX(serial_hash)
		MULTI_MEMBER_INDEX(account_uuid_created_time)
		MULTI_MEMBER_INDEX(expiry_time)
	)

	boost::weak_ptr<AuctionTransactionMapContainer> g_auction_transaction_map;

	void gc_timer_proc(std::uint64_t now){
		PROFILE_ME;
		LOG_EMPERY_CENTER_TRACE("Auction transaction gc timer: now = ", now);

		const auto utc_now = Poseidon::get_utc_time();

		std::vector<boost::shared_ptr<AuctionTransaction>> erased;

		const auto auction_transaction_map = g_auction_transaction_map.lock();
		if(auction_transaction_map){
			erased.reserve(auction_transaction_map->size());

			for(;;){
				const auto it = auction_transaction_map->begin<2>();
				if(it == auction_transaction_map->end<2>()){
					break;
				}
				if(utc_now < it->expiry_time){
					break;
				}
				const auto &auction_transaction = it->auction_transaction;

				LOG_EMPERY_CENTER_DEBUG("Reclaiming auction transaction: serial = ", auction_transaction->get_serial());
				erased.emplace_back(auction_transaction);
				auction_transaction_map->erase<2>(it);
			}
		}

		for(auto it = erased.begin(); it != erased.end(); ++it){
			const auto &auction_transaction = *it;
			try {
				if(!auction_transaction->is_virtually_removed()){
					auction_transaction->cancel("Auction transaction expired");
				}
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
			}
		}
	}

	MODULE_RAII_PRIORITY(handles, 2000){
		const auto conn = Poseidon::MySqlDaemon::create_connection();

		const auto auction_transaction_map = boost::make_shared<AuctionTransactionMapContainer>();
		LOG_EMPERY_CENTER_INFO("Loading auction transactions...");
		const auto utc_now = Poseidon::get_utc_time();
		std::ostringstream oss;
		oss <<"SELECT * FROM `Center_AuctionTransaction` WHERE `expiry_time` > " <<Poseidon::MySql::DateTimeFormatter(utc_now)
		    <<"  AND `cancelled` = 0";
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_AuctionTransaction>();
			obj->fetch(conn);
			obj->enable_auto_saving();
			auto auction_transaction = boost::make_shared<AuctionTransaction>(std::move(obj));
			auction_transaction_map->insert(AuctionTransactionElement(std::move(auction_transaction)));
		}
		LOG_EMPERY_CENTER_INFO("Loaded ", auction_transaction_map->size(), " auction transaction(s).");
		g_auction_transaction_map = auction_transaction_map;
		handles.push(auction_transaction_map);

		const auto gc_interval = get_config<std::uint64_t>("object_gc_interval", 300000);
		auto timer = Poseidon::TimerDaemon::register_timer(0, gc_interval,
			std::bind(&gc_timer_proc, std::placeholders::_2));
		handles.push(timer);
	}
}

boost::shared_ptr<AuctionTransaction> AuctionTransactionMap::get(const std::string &serial){
	PROFILE_ME;

	const auto auction_transaction_map = g_auction_transaction_map.lock();
	if(!auction_transaction_map){
		LOG_EMPERY_CENTER_WARNING("Auction transaction map not loaded.");
		return { };
	}

	const auto range = auction_transaction_map->equal_range<0>(hash_string_nocase(serial));
	for(auto it = range.first; it != range.second; ++it){
		if(are_strings_equal_nocase(it->auction_transaction->get_serial(), serial)){
			return it->auction_transaction;
		}
	}
	LOG_EMPERY_CENTER_TRACE("Auction transaction not found: serial = ", serial);
	return { };
}
boost::shared_ptr<AuctionTransaction> AuctionTransactionMap::require(const std::string &serial){
	PROFILE_ME;

	auto ret = get(serial);
	if(!ret){
		DEBUG_THROW(Exception, sslit("Auction transaction not found"));
	}
	return ret;
}

void AuctionTransactionMap::get_all(std::vector<boost::shared_ptr<AuctionTransaction>> &ret){
	PROFILE_ME;

	const auto auction_transaction_map = g_auction_transaction_map.lock();
	if(!auction_transaction_map){
		LOG_EMPERY_CENTER_WARNING("Auction transaction map not loaded.");
		return;
	}

	ret.reserve(ret.size() + auction_transaction_map->size());
	for(auto it = auction_transaction_map->begin(); it != auction_transaction_map->end(); ++it){
		ret.emplace_back(it->auction_transaction);
	}
}
void AuctionTransactionMap::get_by_account(std::vector<boost::shared_ptr<AuctionTransaction>> &ret, AccountUuid account_uuid){
	PROFILE_ME;

	const auto auction_transaction_map = g_auction_transaction_map.lock();
	if(!auction_transaction_map){
		LOG_EMPERY_CENTER_WARNING("Auction transaction map not loaded.");
		return;
	}

	const auto lower = auction_transaction_map->lower_bound<1>(std::make_pair(account_uuid, (std::uint64_t)0));
	const auto upper = auction_transaction_map->upper_bound<1>(std::make_pair(account_uuid, (std::uint64_t)-1));
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(lower, upper)));
	for(auto it = lower; it != upper; ++it){
		ret.emplace_back(it->auction_transaction);
	}
}

void AuctionTransactionMap::insert(const boost::shared_ptr<AuctionTransaction> &auction_transaction){
	PROFILE_ME;

	const auto auction_transaction_map = g_auction_transaction_map.lock();
	if(!auction_transaction_map){
		LOG_EMPERY_CENTER_WARNING("Auction transaction map not loaded.");
		DEBUG_THROW(Exception, sslit("Auction transaction map not loaded"));
	}

	const auto &serial = auction_transaction->get_serial();

	LOG_EMPERY_CENTER_DEBUG("Inserting auction transaction: serial = ", serial);
	const auto result = auction_transaction_map->insert(AuctionTransactionElement(auction_transaction));
	if(!result.second){
		LOG_EMPERY_CENTER_WARNING("Auction transaction already exists: serial = ", serial);
		DEBUG_THROW(Exception, sslit("Auction transaction already exists"));
	}
}
void AuctionTransactionMap::update(const boost::shared_ptr<AuctionTransaction> &auction_transaction, bool throws_if_not_exists){
	PROFILE_ME;

	const auto auction_transaction_map = g_auction_transaction_map.lock();
	if(!auction_transaction_map){
		LOG_EMPERY_CENTER_WARNING("Auction transaction map not loaded.");
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Auction transaction map not loaded"));
		}
		return;
	}

	const auto &serial = auction_transaction->get_serial();

	const auto range = auction_transaction_map->equal_range<0>(hash_string_nocase(serial));
	auto acit = range.second;
	for(auto it = range.first; it != range.second; ++it){
		if(are_strings_equal_nocase(it->auction_transaction->get_serial(), serial)){
			acit = it;
			break;
		}
	}
	if(acit == range.second){
		LOG_EMPERY_CENTER_WARNING("Auction transaction not found: serial = ", serial);
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Auction transaction not found"));
		}
		return;
	}

	LOG_EMPERY_CENTER_DEBUG("Updating auction transaction: serial = ", serial);
	const auto utc_now = Poseidon::get_utc_time();
	if(auction_transaction->get_expiry_time() < utc_now){
		auction_transaction_map->erase<0>(acit);
	} else {
		auction_transaction_map->replace<0>(acit, AuctionTransactionElement(auction_transaction));
	}
}
void AuctionTransactionMap::remove(const std::string &serial) noexcept {
	PROFILE_ME;

	const auto auction_transaction_map = g_auction_transaction_map.lock();
	if(!auction_transaction_map){
		LOG_EMPERY_CENTER_WARNING("Auction transaction map not loaded.");
		return;
	}

	const auto range = auction_transaction_map->equal_range<0>(hash_string_nocase(serial));
	auto acit = range.second;
	for(auto it = range.first; it != range.second; ++it){
		if(are_strings_equal_nocase(it->auction_transaction->get_serial(), serial)){
			acit = it;
			break;
		}
	}
	if(acit == range.second){
		LOG_EMPERY_CENTER_WARNING("Auction transaction not found: serial = ", serial);
		return;
	}

	LOG_EMPERY_CENTER_DEBUG("Removing auction transaction: serial = ", serial);
	auction_transaction_map->erase<0>(acit);
}

}
