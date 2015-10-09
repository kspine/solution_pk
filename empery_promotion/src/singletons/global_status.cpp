#include "../precompiled.hpp"
#include "global_status.hpp"
#include <boost/container/flat_map.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/singletons/timer_daemon.hpp>
#include "../mysql/global_status.hpp"
#include "../utilities.hpp"
#include "../checked_arithmetic.hpp"
#include "../data/promotion.hpp"
#include "account_map.hpp"

namespace EmperyPromotion {

namespace {
	using StatusMap = boost::container::flat_map<unsigned, boost::shared_ptr<MySql::Promotion_GlobalStatus>>;

	boost::shared_ptr<StatusMap> g_statusMap;

	MODULE_RAII_PRIORITY(handles, 5000){
		const auto conn = Poseidon::MySqlDaemon::createConnection();

		const auto statusMap = boost::make_shared<StatusMap>();
		statusMap->reserve(100);
		LOG_EMPERY_PROMOTION_INFO("Loading global status...");
		conn->executeSql("SELECT * FROM `Promotion_GlobalStatus`");
		while(conn->fetchRow()){
			auto obj = boost::make_shared<MySql::Promotion_GlobalStatus>();
			obj->syncFetch(conn);
			obj->enableAutoSaving();
			statusMap->emplace(obj->get_slot(), std::move(obj));
		}
		LOG_EMPERY_PROMOTION_INFO("Loaded ", statusMap->size(), " global status value(s).");
		g_statusMap = statusMap;
		handles.push(statusMap);

		auto timer = Poseidon::TimerDaemon::registerDailyTimer(0, 1, 0,
			boost::bind(&GlobalStatus::checkDailyReset));
		handles.push(timer);

		GlobalStatus::checkDailyReset();
	}
}

boost::uint64_t GlobalStatus::get(unsigned slot){
	PROFILE_ME;

	const auto it = g_statusMap->find(slot);
	if(it == g_statusMap->end()){
		LOG_EMPERY_PROMOTION_DEBUG("Global status value not found: slot = ", slot);
		return 0;
	}
	return it->second->get_value();
}
boost::uint64_t GlobalStatus::set(unsigned slot, boost::uint64_t newValue){
	PROFILE_ME;

	auto it = g_statusMap->find(slot);
	if(it == g_statusMap->end()){
		auto obj = boost::make_shared<MySql::Promotion_GlobalStatus>(slot, 0);
		obj->asyncSave(true);
		it = g_statusMap->emplace_hint(it, slot, std::move(obj));
	}
	const auto oldValue = it->second->get_value();
	it->second->set_value(newValue);
	return oldValue;
}
boost::uint64_t GlobalStatus::fetchAdd(unsigned slot, boost::uint64_t deltaValue){
	PROFILE_ME;

	auto it = g_statusMap->find(slot);
	if(it == g_statusMap->end()){
		auto obj = boost::make_shared<MySql::Promotion_GlobalStatus>(slot, 0);
		obj->asyncSave(true);
		it = g_statusMap->emplace_hint(it, slot, std::move(obj));
	}
	const auto oldValue = it->second->get_value();
	it->second->set_value(oldValue + deltaValue);
	return oldValue;
}

void GlobalStatus::checkDailyReset(){
	PROFILE_ME;
	LOG_EMPERY_PROMOTION_INFO("Checking for global status daily reset...");

	const auto getObj = [](unsigned slot){
		auto it = g_statusMap->find(slot);
		if(it == g_statusMap->end()){
			auto obj = boost::make_shared<MySql::Promotion_GlobalStatus>(slot, 0);
			obj->asyncSave(true);
			it = g_statusMap->emplace_hint(it, slot, std::move(obj));
		}
		return it->second;
	};

	const auto firstBalancingDelay       = getConfig<boost::uint64_t>("first_balancing_delay",         20 * 86400000);
	const auto accCardUnitPriceIncrement = getConfig<boost::uint64_t>("acc_card_unit_price_increment", 100);
	const auto accCardUnitPriceBegin     = getConfig<boost::uint64_t>("acc_card_unit_price_begin",     40000);
	const auto accCardUnitPriceEnd       = getConfig<boost::uint64_t>("acc_card_unit_price_end",       50000);

	const auto localNow = Poseidon::getLocalTime();

	const auto serverCreatedTimeObj      = getObj(SLOT_SERVER_CREATED_TIME);
	const auto firstBalancingTimeObj     = getObj(SLOT_FIRST_BALANCING_TIME);
	const auto accCardUnitPriceObj       = getObj(SLOT_ACC_CARD_UNIT_PRICE);
	const auto serverDailyResetTimeObj   = getObj(SLOT_SERVER_DAILY_RESET_TIME);

	if(serverCreatedTimeObj->get_value() == 0){
		LOG_EMPERY_PROMOTION_WARNING("This seems to be the first run?");

		auto rootUserName = getConfig<std::string>("init_root_username", "root");
		auto rootNick     = getConfig<std::string>("init_root_nick",     "root");
		auto rootPassword = getConfig<std::string>("init_root_password", "root");

		LOG_EMPERY_PROMOTION_DEBUG("Determining max promotion level...");
		const auto maxPromotionData = Data::Promotion::get(UINT64_MAX);
		if(!maxPromotionData){
			LOG_EMPERY_PROMOTION_ERROR("Failed to determine max promotion level!");
			DEBUG_THROW(Exception, sslit("Failed to determine max promotion level"));
		}

		LOG_EMPERY_PROMOTION_WARNING("Creating root user: rootUserName = ", rootUserName, ", rootPassword = ", rootPassword,
			", level = ", maxPromotionData->level);
		const auto accountId = AccountMap::create(std::move(rootUserName), std::string(), std::move(rootNick),
			rootPassword, rootPassword, AccountId(0), AccountMap::FL_ROBOT);
		LOG_EMPERY_PROMOTION_INFO("> accountId = ", accountId);

		serverCreatedTimeObj->set_value(localNow);
		firstBalancingTimeObj->set_value(localNow / 86400000 * 86400000 + firstBalancingDelay);
		accCardUnitPriceObj->set_value(accCardUnitPriceBegin);
		serverDailyResetTimeObj->set_value(localNow);
		return;
	}

	const auto lastResetTime = serverDailyResetTimeObj->get_value();

	const auto thisDay = localNow / 86400000;
	const auto lastDay = lastResetTime / 86400000;
	if(thisDay <= lastDay){
		return;
	}
	const auto deltaDays = thisDay - lastDay;
	LOG_EMPERY_PROMOTION_INFO("Daily reset: deltaDays = ", deltaDays);

	const auto firstBalancingTime = firstBalancingTimeObj->get_value();

	auto newAccCardUnitPrice = accCardUnitPriceObj->get_value();
	if(localNow >= firstBalancingTime){
		if(lastResetTime < firstBalancingTime){
			LOG_EMPERY_PROMOTION_WARNING("Commit first balance bonus!");
			commitFirstBalanceBonus();
		}

		const auto delatPrice = checkedMul(accCardUnitPriceIncrement, deltaDays);
		LOG_EMPERY_PROMOTION_INFO("Incrementing acceleration card unit price by ", delatPrice);
		newAccCardUnitPrice = checkedAdd(newAccCardUnitPrice, delatPrice);
		if(newAccCardUnitPrice < accCardUnitPriceBegin){
			newAccCardUnitPrice = accCardUnitPriceBegin;
		}
		if(newAccCardUnitPrice > accCardUnitPriceEnd){
			newAccCardUnitPrice = accCardUnitPriceEnd;
		}
		LOG_EMPERY_PROMOTION_INFO("Incremented acceleration card unit price to ", newAccCardUnitPrice);
	}
	accCardUnitPriceObj->set_value(newAccCardUnitPrice);

	serverDailyResetTimeObj->set_value(localNow);
}

}
