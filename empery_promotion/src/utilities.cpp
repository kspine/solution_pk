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
#include <boost/container/flat_map.hpp>
#include <poseidon/string.hpp>

namespace EmperyPromotion {

std::pair<bool, boost::uint64_t> tryUpgradeAccount(AccountId accountId, AccountId payerId, bool isCreatingAccount,
	const boost::shared_ptr<const Data::Promotion> &promotionData, const std::string &remarks)
{
	PROFILE_ME;
	LOG_EMPERY_PROMOTION_INFO("Upgrading account: accountId = ", accountId, ", level = ", promotionData->level);

	const auto oldLevel = AccountMap::castAttribute<boost::uint64_t>(accountId, AccountMap::ATTR_ACCOUNT_LEVEL);
	const auto level = promotionData->level;

	auto levelStr = boost::lexical_cast<std::string>(level);
	AccountMap::touchAttribute(accountId, AccountMap::ATTR_ACCOUNT_LEVEL);

	const double originalUnitPrice = GlobalStatus::get(GlobalStatus::SLOT_ACC_CARD_UNIT_PRICE);
	const boost::uint64_t cardUnitPrice = std::ceil(originalUnitPrice * promotionData->immediateDiscount);
	boost::uint64_t balanceToConsume = 0;
	if(cardUnitPrice != 0){
		const boost::uint64_t cardsToBuy = std::ceil(static_cast<double>(promotionData->immediatePrice) / cardUnitPrice);
		balanceToConsume = checkedMul(cardUnitPrice, cardsToBuy);
		LOG_EMPERY_PROMOTION_INFO("Items: cardsToBuy = ", cardsToBuy, ", balanceToConsume = ", balanceToConsume);

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

	for(auto referrerId = AccountMap::require(accountId).referrerId; referrerId; referrerId = AccountMap::require(referrerId).referrerId){
		const auto referrerLevel = AccountMap::castAttribute<boost::uint64_t>(referrerId, AccountMap::ATTR_ACCOUNT_LEVEL);
		if(referrerLevel == 0){
			LOG_EMPERY_PROMOTION_DEBUG("Referrer is at level zero: referrerId = ", referrerId);
			break;
		}
		auto referrerPromotionData = Data::Promotion::get(referrerLevel);
		if(!referrerPromotionData){
			LOG_EMPERY_PROMOTION_ERROR("No such promotion data: referrerLevel = ", referrerLevel);
			break;
		}
		LOG_EMPERY_PROMOTION_DEBUG("Checking for auto upgrade: referrerId = ", referrerId, ", referrerLevel = ", referrerLevel);

		std::vector<AccountMap::AccountInfo> subordinates;
		AccountMap::getByReferrerId(subordinates, referrerId);
		boost::container::flat_map<boost::uint64_t, std::size_t> levelCounts;
		levelCounts.reserve(32);
		for(auto it = subordinates.begin(); it != subordinates.end(); ++it){
			const auto subordinateLevel = AccountMap::castAttribute<boost::uint64_t>(it->accountId, AccountMap::ATTR_ACCOUNT_LEVEL);
			LOG_EMPERY_PROMOTION_DEBUG("> Subordinate: accountId = ", it->accountId, ", subordinateLevel = ", subordinateLevel);
			++levelCounts[subordinateLevel];
		}
		if(levelCounts.empty()){
			LOG_EMPERY_PROMOTION_DEBUG("No subordinates...");
			break;
		}
		for(;;){
			std::size_t currentAutoUpgradeCount = 0;
			for(auto it = levelCounts.lower_bound(referrerPromotionData->level); it != levelCounts.end(); ++it){
				currentAutoUpgradeCount += it->second;
			}
			LOG_EMPERY_PROMOTION_DEBUG("> Try auto upgrade: level = ", referrerPromotionData->level,
				", currentAutoUpgradeCount = ", currentAutoUpgradeCount, ", autoUpgradeCount = ", referrerPromotionData->autoUpgradeCount);
			if(currentAutoUpgradeCount < referrerPromotionData->autoUpgradeCount){
				LOG_EMPERY_PROMOTION_DEBUG("No enough subordinates");
				break;
			}
			auto nextPromotionData = Data::Promotion::getNext(referrerPromotionData->level);
			if(!nextPromotionData || (nextPromotionData == referrerPromotionData)){
				LOG_EMPERY_PROMOTION_DEBUG("No more promotion levels");
				break;
			}
			referrerPromotionData = std::move(nextPromotionData);
		}
		if(referrerPromotionData->level == referrerLevel){
			LOG_EMPERY_PROMOTION_DEBUG("Not auto upgradeable: referrerId = ", referrerId);
			break;
		}

		LOG_EMPERY_PROMOTION_DEBUG("Auto upgrading: referrerId = ", referrerId, ", level = ", referrerPromotionData->level);
		auto levelStr = boost::lexical_cast<std::string>(referrerPromotionData->level);
		AccountMap::setAttribute(referrerId, AccountMap::ATTR_ACCOUNT_LEVEL, std::move(levelStr));
	}

	return std::make_pair(true, balanceToConsume);
}

namespace {
	double g_bonusRatio;
	std::vector<double> g_incomeTaxRatioArray, g_extraTaxRatioArray;

	MODULE_RAII(){
		auto val = getConfig<double>("balance_bonus_ratio", 0.70);
		LOG_EMPERY_PROMOTION_INFO("> Balance bonus ratio: ", val);
		g_bonusRatio = val;

		auto str = getConfig<std::string>("balance_income_tax_ratio_array", "0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01");
		auto ratioArray = Poseidon::explode<double>(',', str);
		LOG_EMPERY_PROMOTION_INFO("> Balance income tax ratio array: ", Poseidon::implode(',', ratioArray));
		g_incomeTaxRatioArray = std::move(ratioArray);

		str = getConfig<std::string>("balance_extra_tax_ratio_array", "0.02,0.02,0.01");
		ratioArray = Poseidon::explode<double>(',', str);
		LOG_EMPERY_PROMOTION_INFO("> Balance extra tax ratio array: ", Poseidon::implode(',', ratioArray));
		g_extraTaxRatioArray = std::move(ratioArray);
	}

	void reallyAccumulateBalanceBonus(AccountId referrerId, AccountId accountId, AccountId payerId, boost::uint64_t amount){
		PROFILE_ME;
		LOG_EMPERY_PROMOTION_INFO("Balance bonus: referrerId = ", referrerId,
			", accountId = ", accountId, ", payerId = ", payerId, ", amount = ", amount);

	 	std::vector<ItemTransactionElement> transaction;
	 	transaction.reserve(64);

		const auto level = AccountMap::castAttribute<boost::uint64_t>(accountId, AccountMap::ATTR_ACCOUNT_LEVEL);
		const auto promotionData = Data::Promotion::get(level);

		std::deque<std::pair<AccountId, boost::shared_ptr<const Data::Promotion>>> referrers;
		for(auto currentId = referrerId; currentId; currentId = AccountMap::require(currentId).referrerId){
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
					const auto extra = static_cast<boost::uint64_t>(std::floor(dividendTotal * g_extraTaxRatioArray.at(generation)));
					LOG_EMPERY_PROMOTION_DEBUG("> Referrer: referrerId = ", it->first, ", level = ", it->second->level, ", extra = ", extra);
					transaction.emplace_back(it->first, ItemTransactionElement::OP_ADD, ItemIds::ID_ACCOUNT_BALANCE, extra,
						Events::ItemChanged::R_BALANCE_BONUS_EXTRA, accountId.get(), payerId.get(), level, std::string());
					++generation;
				}
			}

			// 扣除个税即使没有上家（视为被系统回收）。
			const auto taxTotal = static_cast<boost::uint64_t>(std::floor(myDividend * incomeTaxRatioTotal));
			transaction.emplace_back(referrerId, ItemTransactionElement::OP_REMOVE, ItemIds::ID_ACCOUNT_BALANCE, taxTotal,
				Events::ItemChanged::R_INCOME_TAX, myDividend, referrerId.get(), 0, std::string());

			generation = 0;
			for(auto it = referrers.begin();(generation < g_incomeTaxRatioArray.size()) && (it != referrers.end()); ++it){
				if(!(it->second)){
					continue;
				}
				const auto tax = static_cast<boost::uint64_t>(std::floor(myDividend * g_incomeTaxRatioArray.at(generation)));
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

	std::vector<AccountMap::AccountInfo> temp;
	temp.reserve(256);
	AccountMap::getByReferrerId(temp, AccountId(0));

	std::deque<AccountMap::AccountInfo> queue;
	std::copy(std::make_move_iterator(temp.begin()), std::make_move_iterator(temp.end()), std::back_inserter(queue));
	while(!queue.empty()){
		const auto &info = queue.front();
		LOG_EMPERY_PROMOTION_DEBUG("> Processing: accountId = ", info.accountId, ", referrerId = ", info.referrerId);
		try {
			temp.clear();
			AccountMap::getByReferrerId(temp, info.accountId);
			for(auto it = temp.begin(); it != temp.end(); ++it){
				const auto level = AccountMap::castAttribute<boost::uint64_t>(it->accountId, AccountMap::ATTR_ACCOUNT_LEVEL);
				reallyAccumulateBalanceBonus(info.accountId, it->accountId, it->accountId, level);
			}
		} catch(std::exception &e){
			LOG_EMPERY_PROMOTION_ERROR("std::exception thrown: what = ", e.what());
		}
		queue.pop_front();
	}
}
void accumulateBalanceBonus(AccountId accountId, AccountId payerId, boost::uint64_t amount){
	PROFILE_ME;

	const auto referrerId = AccountMap::require(accountId).referrerId;
	if(!referrerId){
		LOG_EMPERY_PROMOTION_DEBUG("No referrer: accountId = ", accountId);
		return;
	}

	const auto localNow = Poseidon::getLocalTime();
	const auto firstBalancingTime = GlobalStatus::get(GlobalStatus::SLOT_FIRST_BALANCING_TIME);
	if(localNow < firstBalancingTime){
		LOG_EMPERY_PROMOTION_DEBUG("Before first balancing...");
		return;
	}

	reallyAccumulateBalanceBonus(referrerId, accountId, payerId, amount);
}

std::string generateBillSerial(const std::string &prefix){
	PROFILE_ME;

	const auto autoInc = GlobalStatus::get(GlobalStatus::SLOT_BILL_AUTO_INC);
	GlobalStatus::set(GlobalStatus::SLOT_BILL_AUTO_INC, autoInc + 1);

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
