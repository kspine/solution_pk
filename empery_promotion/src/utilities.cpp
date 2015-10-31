#include "precompiled.hpp"
#include "utilities.hpp"
#include "singletons/account_map.hpp"
#include "singletons/item_map.hpp"
#include "singletons/global_status.hpp"
#include "data/promotion.hpp"
#include "events/item.hpp"
#include "item_transaction_element.hpp"
#include "item_ids.hpp"
#include "checked_arithmetic.hpp"
#include "mysql/bill.hpp"
#include "bill_states.hpp"
#include <boost/container/flat_map.hpp>
#include <poseidon/string.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include <string>

namespace EmperyPromotion {

namespace {
	struct StringCaseComparator {
		bool operator()(const std::string &lhs, const std::string &rhs) const {
			return ::strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
		}
	};
}

std::pair<bool, boost::uint64_t> tryUpgradeAccount(AccountId accountId, AccountId payerId, bool isCreatingAccount,
	const boost::shared_ptr<const Data::Promotion> &promotionData, const std::string &remarks, boost::uint64_t additionalCards)
{
	PROFILE_ME;
	LOG_EMPERY_PROMOTION_INFO("Upgrading account: accountId = ", accountId, ", level = ", promotionData->level);

	const auto oldLevel = AccountMap::castAttribute<boost::uint64_t>(accountId, AccountMap::ATTR_ACCOUNT_LEVEL);
	const auto level = promotionData->level;

	auto info = AccountMap::require(accountId);

	auto levelStr = boost::lexical_cast<std::string>(level);
	AccountMap::touchAttribute(accountId, AccountMap::ATTR_ACCOUNT_LEVEL);

	const double originalUnitPrice = GlobalStatus::get(GlobalStatus::SLOT_ACC_CARD_UNIT_PRICE);
	const boost::uint64_t cardUnitPrice = std::ceil(originalUnitPrice * promotionData->immediateDiscount);
	boost::uint64_t balanceToConsume = 0;
	if(cardUnitPrice != 0){
		const auto minCardsToBuy = static_cast<boost::uint64_t>(
			std::ceil(static_cast<double>(promotionData->immediatePrice) / cardUnitPrice));
		const auto cardsToBuy = checkedAdd(minCardsToBuy, additionalCards);
		balanceToConsume = checkedMul(cardUnitPrice, cardsToBuy);
		LOG_EMPERY_PROMOTION_INFO("Acceleration cards: minCardsToBuy = ", minCardsToBuy, ", additionalCards = ", additionalCards,
			", cardsToBuy = ", cardsToBuy, ", balanceToConsume = ", balanceToConsume);

		auto reason = Events::ItemChanged::R_UPGRADE_ACCOUNT;
		if(isCreatingAccount){
			reason = Events::ItemChanged::R_CREATE_ACCOUNT;
		}

		std::vector<ItemTransactionElement> transaction;
		transaction.emplace_back(accountId, ItemTransactionElement::OP_ADD, ItemIds::ID_ACCELERATION_CARDS, cardsToBuy,
			reason, accountId.get(), payerId.get(), level, remarks);
		transaction.emplace_back(payerId, ItemTransactionElement::OP_REMOVE, ItemIds::ID_ACCOUNT_BALANCE, balanceToConsume,
			reason, accountId.get(), payerId.get(), level, remarks);
		const auto insufficientItemId = ItemMap::commitTransactionNoThrow(transaction.data(), transaction.size());
		if(insufficientItemId){
			return std::make_pair(false, balanceToConsume);
		}
	}

	if(oldLevel < level){
		AccountMap::setAttribute(accountId, AccountMap::ATTR_ACCOUNT_LEVEL, std::move(levelStr));
	}

	std::deque<std::pair<AccountId, boost::shared_ptr<const Data::Promotion>>> referrers;
	for(auto currentId = info.referrerId; currentId; currentId = AccountMap::require(currentId).referrerId){
		const auto referrerLevel = AccountMap::castAttribute<boost::uint64_t>(currentId, AccountMap::ATTR_ACCOUNT_LEVEL);
		LOG_EMPERY_PROMOTION_DEBUG("> Next referrer: currentId = ", currentId, ", referrerLevel = ", referrerLevel);
		auto referrerPromotionData = Data::Promotion::get(referrerLevel);
		referrers.emplace_back(currentId, std::move(referrerPromotionData));
	}

	boost::container::flat_map<boost::uint64_t, boost::uint64_t> subordinateLevelCounts;
	subordinateLevelCounts.reserve(32);
	while(!referrers.empty()){
		const auto referrerId = referrers.front().first;
		const auto referrerPromotionData = std::move(referrers.front().second);
		referrers.pop_front();

		if(!referrerPromotionData){
			LOG_EMPERY_PROMOTION_DEBUG("> Referrer is at level zero: referrerId = ", referrerId);
			continue;
		}
		LOG_EMPERY_PROMOTION_DEBUG("> Checking for auto upgrade: referrerId = ", referrerId, ", level = ", referrerPromotionData->level);

		std::vector<AccountMap::AccountInfo> subordinates;
		AccountMap::getByReferrerId(subordinates, referrerId);
		for(auto it = subordinates.begin(); it != subordinates.end(); ++it){
			const auto subordinateLevel = AccountMap::castAttribute<boost::uint64_t>(it->accountId, AccountMap::ATTR_ACCOUNT_LEVEL);
			LOG_EMPERY_PROMOTION_DEBUG("> Subordinate: accountId = ", it->accountId, ", subordinateLevel = ", subordinateLevel);
			++subordinateLevelCounts[subordinateLevel];
		}
		auto nextPromotionData = referrerPromotionData;
		for(;;){
			std::size_t currentAutoUpgradeCount = 0;
			for(auto it = subordinateLevelCounts.lower_bound(nextPromotionData->level); it != subordinateLevelCounts.end(); ++it){
				currentAutoUpgradeCount += it->second;
			}
			LOG_EMPERY_PROMOTION_DEBUG("> Try auto upgrade: level = ", nextPromotionData->level,
				", currentAutoUpgradeCount = ", currentAutoUpgradeCount, ", autoUpgradeCount = ", nextPromotionData->autoUpgradeCount);
			if(currentAutoUpgradeCount < nextPromotionData->autoUpgradeCount){
				LOG_EMPERY_PROMOTION_DEBUG("No enough subordinates");
				break;
			}
			auto testPromotionData = Data::Promotion::getNext(nextPromotionData->level);
			if(!testPromotionData || (testPromotionData == nextPromotionData)){
				LOG_EMPERY_PROMOTION_DEBUG("No more promotion levels");
				break;
			}
			nextPromotionData = std::move(testPromotionData);
		}
		if(nextPromotionData == referrerPromotionData){
			LOG_EMPERY_PROMOTION_DEBUG("Not auto upgradeable: referrerId = ", referrerId);
		} else {
			LOG_EMPERY_PROMOTION_DEBUG("Auto upgrading: referrerId = ", referrerId, ", level = ", referrerPromotionData->level);
			auto levelStr = boost::lexical_cast<std::string>(referrerPromotionData->level);
			AccountMap::setAttribute(referrerId, AccountMap::ATTR_ACCOUNT_LEVEL, std::move(levelStr));
		}
	}

	return std::make_pair(true, balanceToConsume);
}

namespace {
	double g_bonusRatio;
	std::vector<double> g_incomeTaxRatioArray, g_extraTaxRatioArray;

	MODULE_RAII_PRIORITY(/* handles */, 1000){
		auto val = getConfig<double>("balance_bonus_ratio", 0.70);
		LOG_EMPERY_PROMOTION_DEBUG("> Balance bonus ratio: ", val);
		g_bonusRatio = val;

		auto str = getConfig<std::string>("balance_income_tax_ratio_array", "0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01");
		auto ratioArray = Poseidon::explode<double>(',', str);
		LOG_EMPERY_PROMOTION_DEBUG("> Balance income tax ratio array: ", Poseidon::implode(',', ratioArray));
		g_incomeTaxRatioArray = std::move(ratioArray);

		str = getConfig<std::string>("balance_extra_tax_ratio_array", "0.02,0.02,0.01");
		ratioArray = Poseidon::explode<double>(',', str);
		LOG_EMPERY_PROMOTION_DEBUG("> Balance extra tax ratio array: ", Poseidon::implode(',', ratioArray));
		g_extraTaxRatioArray = std::move(ratioArray);
	}

	void reallyAccumulateBalanceBonus(AccountId accountId, AccountId payerId, boost::uint64_t amount){
		PROFILE_ME;
		LOG_EMPERY_PROMOTION_INFO("Balance bonus: accountId = ", accountId, ", payerId = ", payerId, ", amount = ", amount);

	 	std::vector<ItemTransactionElement> transaction;
	 	transaction.reserve(64);

		const auto level = AccountMap::castAttribute<boost::uint64_t>(accountId, AccountMap::ATTR_ACCOUNT_LEVEL);
		const auto promotionData = Data::Promotion::get(level);

		AccountId virtualFirstReferrerId;
		const auto localNow = Poseidon::getLocalTime();
		const auto firstBalancingTime = GlobalStatus::get(GlobalStatus::SLOT_FIRST_BALANCING_TIME);
		if(localNow < firstBalancingTime){
			LOG_EMPERY_PROMOTION_DEBUG("Before first balancing...");
			virtualFirstReferrerId = accountId;
		} else {
			const auto info = AccountMap::require(accountId);
			virtualFirstReferrerId = info.referrerId;
		}

		std::deque<std::pair<AccountId, boost::shared_ptr<const Data::Promotion>>> referrers;
		for(auto currentId = virtualFirstReferrerId; currentId; currentId = AccountMap::require(currentId).referrerId){
			const auto referrerLevel = AccountMap::castAttribute<boost::uint64_t>(currentId, AccountMap::ATTR_ACCOUNT_LEVEL);
			LOG_EMPERY_PROMOTION_DEBUG("> Next referrer: currentId = ", currentId, ", referrerLevel = ", referrerLevel);
			auto referrerPromotionData = Data::Promotion::get(referrerLevel);
			referrers.emplace_back(currentId, std::move(referrerPromotionData));
		}

		const auto dividendTotal = static_cast<boost::uint64_t>(amount * g_bonusRatio);
		const auto incomeTaxRatioTotal = std::accumulate(g_incomeTaxRatioArray.begin(), g_incomeTaxRatioArray.end(), 0.0);

		boost::uint64_t dividendAccumulated = 0;
		while(!referrers.empty()){
			const auto referrerId = referrers.front().first;
			const auto referrerPromotionData = std::move(referrers.front().second);
			referrers.pop_front();

			if(!referrerPromotionData){
				LOG_EMPERY_PROMOTION_DEBUG("> Referrer is at level zero: referrerId = ", referrerId);
				continue;
			}
			const auto myMaxDividend = dividendTotal * referrerPromotionData->taxRatio;
			LOG_EMPERY_PROMOTION_DEBUG("> Current referrer: referrerId = ", referrerId,
				", level = ", referrerPromotionData->level, ", taxRatio = ", referrerPromotionData->taxRatio,
				", dividendAccumulated = ", dividendAccumulated, ", myMaxDividend = ", myMaxDividend);

			// 级差制。
			if(dividendAccumulated >= myMaxDividend){
				continue;
			}
			const auto myDividend = myMaxDividend - dividendAccumulated;
			dividendAccumulated = myMaxDividend;

			transaction.emplace_back(referrerId, ItemTransactionElement::OP_ADD, ItemIds::ID_ACCOUNT_BALANCE, myDividend,
				Events::ItemChanged::R_BALANCE_BONUS, accountId.get(), payerId.get(), level, std::string());

			unsigned generation;

			if(referrerPromotionData && referrerPromotionData->taxExtra){
				generation = 0;
				for(auto it = referrers.begin(); (generation < g_extraTaxRatioArray.size()) && (it != referrers.end()); ++it){
					if(!(it->second)){
						continue;
					}
					LOG_EMPERY_PROMOTION_DEBUG("> Referrer: referrerId = ", it->first, ", level = ", it->second->level);
					if(!(it->second->taxExtra)){
						LOG_EMPERY_PROMOTION_DEBUG("> No extra tax available.");
						continue;
					}
					const auto extra = static_cast<boost::uint64_t>(std::round(dividendTotal * g_extraTaxRatioArray.at(generation)));
					LOG_EMPERY_PROMOTION_DEBUG("> Referrer: referrerId = ", it->first, ", level = ", it->second->level, ", extra = ", extra);
					transaction.emplace_back(it->first, ItemTransactionElement::OP_ADD, ItemIds::ID_ACCOUNT_BALANCE, extra,
						Events::ItemChanged::R_BALANCE_BONUS_EXTRA, accountId.get(), payerId.get(), level, std::string());
					++generation;
				}
			}

			// 扣除个税即使没有上家（视为被系统回收）。
			const auto taxTotal = static_cast<boost::uint64_t>(std::round(myDividend * incomeTaxRatioTotal));
			transaction.emplace_back(referrerId, ItemTransactionElement::OP_REMOVE, ItemIds::ID_ACCOUNT_BALANCE, taxTotal,
				Events::ItemChanged::R_INCOME_TAX, myDividend, referrerId.get(), 0, std::string());

			generation = 0;
			for(auto it = referrers.begin(); (generation < g_incomeTaxRatioArray.size()) && (it != referrers.end()); ++it){
				if(!(it->second)){
					continue;
				}
				const auto tax = static_cast<boost::uint64_t>(std::round(myDividend * g_incomeTaxRatioArray.at(generation)));
				LOG_EMPERY_PROMOTION_DEBUG("> Referrer: referrerId = ", it->first, ", level = ", it->second->level, ", tax = ", tax);
				transaction.emplace_back(it->first, ItemTransactionElement::OP_ADD, ItemIds::ID_ACCOUNT_BALANCE, tax,
					Events::ItemChanged::R_INCOME_TAX, myDividend, referrerId.get(), 0, std::string());
				++generation;
			}
		}

		ItemMap::commitTransaction(transaction.data(), transaction.size());
		LOG_EMPERY_PROMOTION_INFO("Done!");
	}
}

void commitFirstBalanceBonus(){
	PROFILE_ME;
	LOG_EMPERY_PROMOTION_INFO("Committing first balancing...");

	std::map<std::string, boost::shared_ptr<const Data::Promotion>, StringCaseComparator> toutAccounts;
	{
		auto vec = getConfigV<std::string>("tout_account");
		for(auto it = vec.begin(); it != vec.end(); ++it){
			auto parts = Poseidon::explode<std::string>(',', *it, 2);
			auto loginName = std::move(parts.at(0));
			auto level = boost::lexical_cast<boost::uint64_t>(parts.at(1));
			auto promotionData = Data::Promotion::get(level);
			if(!promotionData){
				LOG_EMPERY_PROMOTION_ERROR("Level not found: level = ", level);
				continue;
			}
			LOG_EMPERY_PROMOTION_DEBUG("> Tout account: loginName = ", loginName, ", level = ", promotionData->level);
			toutAccounts[std::move(loginName)] = std::move(promotionData);
		}
	}

	std::map<AccountId, std::deque<boost::uint64_t>> rechargeHistory;
	{
		std::vector<boost::shared_ptr<MySql::Promotion_Bill>> tempBillObjs;
		std::ostringstream oss;
		oss <<"SELECT * FROM `Promotion_Bill` WHERE `state` = " <<(unsigned)BillStates::ST_SETTLED <<" ORDER BY `serial` DESC";
		MySql::Promotion_Bill::batchLoad(tempBillObjs, oss.str());
		for(auto it = tempBillObjs.begin(); it != tempBillObjs.end(); ++it){
			const auto &obj = *it;
			rechargeHistory[AccountId(obj->get_accountId())].push_back(obj->get_amount());
		}
	}

	std::vector<AccountMap::AccountInfo> tempInfos;
	tempInfos.reserve(256);

	std::deque<AccountMap::AccountInfo> queue;
	AccountMap::getByReferrerId(tempInfos, AccountId(0));
	std::copy(std::make_move_iterator(tempInfos.begin()), std::make_move_iterator(tempInfos.end()), std::back_inserter(queue));
	while(!queue.empty()){
		const auto info = std::move(queue.front());
		queue.pop_front();
		LOG_EMPERY_PROMOTION_TRACE("> Processing: accountId = ", info.accountId, ", referrerId = ", info.referrerId);
		try {
			tempInfos.clear();
			AccountMap::getByReferrerId(tempInfos, info.accountId);
			std::copy(std::make_move_iterator(tempInfos.begin()), std::make_move_iterator(tempInfos.end()), std::back_inserter(queue));
		} catch(std::exception &e){
			LOG_EMPERY_PROMOTION_ERROR("std::exception thrown: what = ", e.what());
		}

		try {
			std::string newLevel;
			const auto it = toutAccounts.find(info.loginName);
			if(it != toutAccounts.end()){
				newLevel = boost::lexical_cast<std::string>(it->second->level);
			}
			AccountMap::setAttribute(info.accountId, AccountMap::ATTR_ACCOUNT_LEVEL, std::move(newLevel));
		} catch(std::exception &e){
			LOG_EMPERY_PROMOTION_ERROR("std::exception thrown: what = ", e.what());
		}

		const auto queueIt = rechargeHistory.find(info.accountId);
		if(queueIt == rechargeHistory.end()){
			continue;
		}
		while(!queueIt->second.empty()){
			const auto amount = queueIt->second.front();
			queueIt->second.pop_front();
			try {
				LOG_EMPERY_PROMOTION_DEBUG("> Accumulating: accountId = ", info.accountId, ", amount = ", amount);
				reallyAccumulateBalanceBonus(info.accountId, info.accountId, amount);
			} catch(std::exception &e){
				LOG_EMPERY_PROMOTION_ERROR("std::exception thrown: what = ", e.what());
			}
		}
		rechargeHistory.erase(queueIt);
	}
}
void accumulateBalanceBonus(AccountId accountId, AccountId payerId, boost::uint64_t amount){
	PROFILE_ME;

	const auto localNow = Poseidon::getLocalTime();
	const auto firstBalancingTime = GlobalStatus::get(GlobalStatus::SLOT_FIRST_BALANCING_TIME);
	if(localNow < firstBalancingTime){
		LOG_EMPERY_PROMOTION_DEBUG("Before first balancing...");
		return;
	}

	reallyAccumulateBalanceBonus(accountId, payerId, amount);
}

std::string generateBillSerial(const std::string &prefix){
	PROFILE_ME;

	const auto autoInc = GlobalStatus::fetchAdd(GlobalStatus::SLOT_BILL_AUTO_INC, 1);

	std::string serial;
	serial.reserve(255);
	serial.append(prefix);
	const auto localNow = Poseidon::getLocalTime();
	const auto dt = Poseidon::breakDownTime(localNow);
	char temp[256];
	unsigned len = (unsigned)std::sprintf(temp, "%04u%02u%02u%02u", dt.yr, dt.mon, dt.day, dt.hr);
	serial.append(temp, len);
	len = (unsigned)std::sprintf(temp, "%09u", (unsigned)autoInc);
	serial.append(temp + len - 6, 6);
	return serial;
}

}
