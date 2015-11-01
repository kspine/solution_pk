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
#include "mysql/outcome_balance_history.hpp"
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

	void checkAutoUpgradeable(AccountId initAccountId){
		PROFILE_ME;

		auto nextAccountId = initAccountId;
		while(nextAccountId){
			const auto accountId = nextAccountId;
			nextAccountId = AccountMap::require(accountId).referrerId;

			const auto level = AccountMap::castAttribute<boost::uint64_t>(accountId, AccountMap::ATTR_ACCOUNT_LEVEL);
			LOG_EMPERY_PROMOTION_DEBUG("> Next referrer: accountId = ", accountId, ", level = ", level);
			const auto oldPromotionData = Data::Promotion::get(level);
			if(!oldPromotionData){
				LOG_EMPERY_PROMOTION_DEBUG("> Referrer is at level zero.");
				continue;
			}

			boost::container::flat_map<boost::uint64_t, boost::uint64_t> subordinateLevelCounts;
			subordinateLevelCounts.reserve(32);
			{
				std::vector<AccountMap::AccountInfo> subordinates;
				AccountMap::getByReferrerId(subordinates, accountId);
				for(auto it = subordinates.begin(); it != subordinates.end(); ++it){
					const auto maxSubordLevel = AccountMap::castAttribute<boost::uint64_t>(it->accountId, AccountMap::ATTR_MAX_SUBORD_LEVEL);
					LOG_EMPERY_PROMOTION_DEBUG("> > Subordinate: accountId = ", it->accountId, ", maxSubordLevel = ", maxSubordLevel);
					++subordinateLevelCounts[maxSubordLevel];
				}
			}
			auto newPromotionData = oldPromotionData;
			for(;;){
				std::size_t count = 0;
				for(auto it = subordinateLevelCounts.lower_bound(newPromotionData->level); it != subordinateLevelCounts.end(); ++it){
					count += it->second;
				}
				LOG_EMPERY_PROMOTION_DEBUG("> Try auto upgrade: level = ", newPromotionData->level,
					", count = ", count, ", autoUpgradeCount = ", newPromotionData->autoUpgradeCount);
				if(count < newPromotionData->autoUpgradeCount){
					LOG_EMPERY_PROMOTION_DEBUG("No enough subordinates");
					break;
				}
				auto promotionData = Data::Promotion::getNext(newPromotionData->level);
				if(!promotionData || (promotionData == newPromotionData)){
					LOG_EMPERY_PROMOTION_DEBUG("No more promotion levels");
					break;
				}
				newPromotionData = std::move(promotionData);
			}
			if(newPromotionData == oldPromotionData){
				LOG_EMPERY_PROMOTION_DEBUG("Not auto upgradeable: accountId = ", accountId);
			} else {
				LOG_EMPERY_PROMOTION_DEBUG("Auto upgrading: accountId = ", accountId, ", level = ", newPromotionData->level);
				AccountMap::setAttribute(accountId, AccountMap::ATTR_ACCOUNT_LEVEL, boost::lexical_cast<std::string>(newPromotionData->level));
			}
		}
	}
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
	checkAutoUpgradeable(info.referrerId);

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

	void reallyAccumulateBalanceBonus(AccountId accountId, AccountId payerId, boost::uint64_t amount,
		AccountId virtualReferrerId, boost::uint64_t upgradeToLevel)
	{
		PROFILE_ME;
		LOG_EMPERY_PROMOTION_INFO("Balance bonus: accountId = ", accountId, ", payerId = ", payerId, ", amount = ", amount,
			", virtualReferrerId = ", virtualReferrerId, ", upgradeToLevel = ", upgradeToLevel);

		std::vector<ItemTransactionElement> transaction;
		transaction.reserve(64);

		const auto firstPromotionData = Data::Promotion::getFirst();

		std::deque<std::pair<AccountId, boost::shared_ptr<const Data::Promotion>>> referrers;
		for(auto currentId = virtualReferrerId; currentId; currentId = AccountMap::require(currentId).referrerId){
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
				Events::ItemChanged::R_BALANCE_BONUS, accountId.get(), payerId.get(), upgradeToLevel, std::string());

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
						Events::ItemChanged::R_BALANCE_BONUS_EXTRA, accountId.get(), payerId.get(), upgradeToLevel, std::string());
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
				if(firstPromotionData && (it->second->level <= firstPromotionData->level)){
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

	std::map<AccountId, std::deque<boost::shared_ptr<MySql::Promotion_OutcomeBalanceHistory>>> rechargeHistory;
	{
		std::vector<boost::shared_ptr<MySql::Promotion_OutcomeBalanceHistory>> tempObjs;
		MySql::Promotion_OutcomeBalanceHistory::batchLoad(tempObjs,
			"SELECT * FROM `Promotion_OutcomeBalanceHistory` ORDER BY `timestamp` ASC, `autoId` ASC");
		for(auto it = tempObjs.begin(); it != tempObjs.end(); ++it){
			auto &obj = *it;
			const auto accountId = AccountId(obj->get_accountId());
			rechargeHistory[accountId].emplace_back(std::move(obj));
		}
	}

	std::set<std::string, StringCaseComparator> fakeOutcome;
	{
		auto fakeOutcomeVec = getConfigV<std::string>("fake_outcome");
		for(auto it = fakeOutcomeVec.begin(); it != fakeOutcomeVec.end(); ++it){
			fakeOutcome.insert(std::move(*it));
		}
	}

	const auto firstLevelData = Data::Promotion::getFirst();
	if(!firstLevelData){
		LOG_EMPERY_PROMOTION_FATAL("No first level?");
		std::abort();
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
			std::string newLevelStr;
			const auto it = toutAccounts.find(info.loginName);
			if(it != toutAccounts.end()){
				newLevelStr = boost::lexical_cast<std::string>(it->second->level);
			} else {
				newLevelStr = boost::lexical_cast<std::string>(firstLevelData->level);
			}
			AccountMap::setAttribute(info.accountId, AccountMap::ATTR_ACCOUNT_LEVEL, std::move(newLevelStr));
			checkAutoUpgradeable(info.referrerId);
		} catch(std::exception &e){
			LOG_EMPERY_PROMOTION_ERROR("std::exception thrown: what = ", e.what());
		}

		const auto queueIt = rechargeHistory.find(info.accountId);
		if(queueIt != rechargeHistory.end()){
			while(!queueIt->second.empty()){
				const auto obj = std::move(queueIt->second.front());
				queueIt->second.pop_front();

				if((obj->get_reason() != Events::ItemChanged::R_UPGRADE_ACCOUNT) &&
					(obj->get_reason() != Events::ItemChanged::R_CREATE_ACCOUNT))
				{
					continue;
				}
				try {
					LOG_EMPERY_PROMOTION_DEBUG("> Accumulating: accountId = ", info.accountId,
						", outcomeBalance = ", obj->get_outcomeBalance(), ", reason = ", obj->get_reason());
					const auto oldLevel = AccountMap::castAttribute<boost::uint64_t>(info.accountId, AccountMap::ATTR_ACCOUNT_LEVEL);
					const auto level = obj->get_param3();
					if(fakeOutcome.find(info.loginName) != fakeOutcome.end()){
						LOG_EMPERY_PROMOTION_DEBUG("> Fake outcome user: loginName = ", info.loginName);
					} else {
						reallyAccumulateBalanceBonus(info.accountId, info.accountId, obj->get_outcomeBalance(),
							// 首次结算从自己开始，以后从推荐人开始。
							info.accountId, level);
					}
					if(oldLevel < level){
						AccountMap::setAttribute(info.accountId, AccountMap::ATTR_ACCOUNT_LEVEL, boost::lexical_cast<std::string>(level));
					}
					checkAutoUpgradeable(info.referrerId);
				} catch(std::exception &e){
					LOG_EMPERY_PROMOTION_ERROR("std::exception thrown: what = ", e.what());
				}
			}
			rechargeHistory.erase(queueIt);
		}
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

	const auto info = AccountMap::require(accountId);
	const auto level = AccountMap::castAttribute<boost::uint64_t>(info.accountId, AccountMap::ATTR_ACCOUNT_LEVEL);
	reallyAccumulateBalanceBonus(accountId, payerId, amount,
		info.referrerId, level);
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
