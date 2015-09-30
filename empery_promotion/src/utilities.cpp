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
	const boost::shared_ptr<const Data::Promotion> &promotionData, std::string remarks)
{
	PROFILE_ME;
	LOG_EMPERY_PROMOTION_INFO("Upgrading account: accountId = ", accountId, ", price = ", promotionData->price);

	const auto oldLevel = AccountMap::castAttribute<boost::uint64_t>(accountId, AccountMap::ATTR_ACCOUNT_LEVEL);
	const auto level = promotionData->price;

	auto levelStr = boost::lexical_cast<std::string>(level);
	AccountMap::touchAttribute(accountId, AccountMap::ATTR_ACCOUNT_LEVEL);

	const double originalUnitPrice = GlobalStatus::get(GlobalStatus::SLOT_ACC_CARD_UNIT_PRICE);
	const boost::uint64_t cardUnitPrice = std::ceil(originalUnitPrice * promotionData->immediateDiscount);
	boost::uint64_t balanceToConsume = 0;
	if(cardUnitPrice != 0){
		const boost::uint64_t cardsToBuy = std::ceil(static_cast<double>(promotionData->immediatePrice) / cardUnitPrice);
		balanceToConsume = checkedMul(cardUnitPrice, cardsToBuy);
		LOG_EMPERY_PROMOTION_INFO("Items: cardsToBuy = ", cardsToBuy, ", balanceToConsume = ", balanceToConsume);

		std::vector<ItemTransactionElement> transaction;
		transaction.emplace_back(accountId,
			ItemTransactionElement::OP_ADD, ItemIds::ID_ACCELERATION_CARDS, cardsToBuy);
		transaction.emplace_back(payerId,
			ItemTransactionElement::OP_REMOVE, ItemIds::ID_ACCOUNT_BALANCE, balanceToConsume);
		auto reason = Events::ItemChanged::R_UPGRADE_ACCOUNT;
		if(isCreatingAccount){
			reason = Events::ItemChanged::R_CREATE_ACCOUNT;
		}
		const auto insufficientItemId = ItemMap::commitTransactionNoThrow(transaction.data(), transaction.size(),
			reason, accountId.get(), payerId.get(), promotionData->price, std::move(remarks));
		if(insufficientItemId){
			return std::make_pair(false, balanceToConsume);
		}
	}

	if(oldLevel < level){
		AccountMap::setAttribute(accountId, AccountMap::ATTR_ACCOUNT_LEVEL, std::move(levelStr));
	}

	auto referrerId = AccountMap::require(accountId).referrerId;
	while(referrerId){
		const auto referrerLevel = AccountMap::castAttribute<boost::uint64_t>(referrerId, AccountMap::ATTR_ACCOUNT_LEVEL);
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
		auto levelIt = levelCounts.end();
		--levelIt;
		boost::shared_ptr<const Data::Promotion> upgradeableToPromotionData;
		for(;;){
			const auto subordinateLevel = levelIt->first;
			const auto subordinateCount = levelIt->second;
			LOG_EMPERY_PROMOTION_DEBUG("> Checking: subordinateLevel = ", subordinateLevel, ", subordinateCount = ", subordinateCount);
			upgradeableToPromotionData = Data::Promotion::get(subordinateLevel + 1);
			if(upgradeableToPromotionData && (subordinateCount >= upgradeableToPromotionData->prevLevelCountToAutoUpgrade)){
				LOG_EMPERY_PROMOTION_DEBUG("Auto upgradeable: price = ", upgradeableToPromotionData->price,
					", subordinateLevel = ", subordinateLevel, ", subordinateCount = ", subordinateCount);
				break;
			}
			levelCounts.erase(levelIt);
			if(levelCounts.empty()){
				break;
			}
			levelIt = levelCounts.end();
			--levelIt;
			levelIt->second += subordinateCount;
		}
		if(!upgradeableToPromotionData){
			LOG_EMPERY_PROMOTION_DEBUG("Not auto upgradeable: referrerId = ", referrerId);
			break;
		}
		if(upgradeableToPromotionData->price <= referrerLevel){
			LOG_EMPERY_PROMOTION_DEBUG("Already auto upgraded: referrerId = ", referrerId);
			break;
		}
		levelStr = boost::lexical_cast<std::string>(upgradeableToPromotionData->price);
		AccountMap::setAttribute(referrerId, AccountMap::ATTR_ACCOUNT_LEVEL, std::move(levelStr));

		referrerId = AccountMap::require(referrerId).referrerId;
	}

	return std::make_pair(true, balanceToConsume);
}

namespace {
	void reallyAccumulateBalanceBonus(AccountId referrerId, AccountId accountId, AccountId payerId, boost::uint64_t amount,
		double levelRatioTotal, const std::vector<double> &extraRatioArray, double generationRatio, unsigned generationCount)
	{
	 	std::vector<ItemTransactionElement> transaction;
	 	transaction.reserve(16);

		std::vector<std::pair<AccountId, boost::shared_ptr<const Data::Promotion>>> referrers;
		referrers.reserve(16);
		for(auto currentId = referrerId; currentId; currentId = AccountMap::require(currentId).referrerId){
			const auto referrerLevel = AccountMap::castAttribute<boost::uint64_t>(currentId, AccountMap::ATTR_ACCOUNT_LEVEL);
			LOG_EMPERY_PROMOTION_DEBUG("> Next referrer: currentId = ", currentId, ", referrerLevel = ", referrerLevel);
			auto referrerPromotionData = Data::Promotion::get(referrerLevel);
			referrers.emplace_back(currentId, std::move(referrerPromotionData));
		}

		const auto levelDividendTotal = static_cast<boost::uint64_t>(amount * levelRatioTotal);
		boost::uint64_t dividendAccumulated = 0;
		for(auto referrerIt = referrers.begin(); (referrerIt != referrers.end()) && (dividendAccumulated < levelDividendTotal); ++referrerIt){
			const auto currentReferrerId = referrerIt->first;
			const auto &referrerPromotionData = referrerIt->second;

			if(!referrerPromotionData){
				continue;
			}
			const auto maxDividendForReferrer = static_cast<boost::uint64_t>(levelDividendTotal * referrerPromotionData->taxRatio);
			if(maxDividendForReferrer < dividendAccumulated){
				continue;
			}
			const auto realDividendForReferrer = maxDividendForReferrer - dividendAccumulated;
			LOG_EMPERY_PROMOTION_DEBUG("> Level bonus: currentReferrerId = ", currentReferrerId,
				", realDividendForReferrer = ", realDividendForReferrer);
			transaction.emplace_back(currentReferrerId,
				ItemTransactionElement::OP_ADD, ItemIds::ID_ACCOUNT_BALANCE, realDividendForReferrer);
			dividendAccumulated = maxDividendForReferrer;
		}

		const auto accountLevel = AccountMap::castAttribute<boost::uint64_t>(accountId, AccountMap::ATTR_ACCOUNT_LEVEL);
		const auto promotionData = Data::Promotion::get(accountLevel);
		if(promotionData->taxExtra){
			auto extraIt = extraRatioArray.begin();
			for(auto referrerIt = referrers.begin(); (referrerIt != referrers.end()) && (extraIt != extraRatioArray.end()); ++referrerIt){
				const auto currentReferrerId = referrerIt->first;
				const auto &referrerPromotionData = referrerIt->second;

				if(!referrerPromotionData){
					continue;
				}
				if(!referrerPromotionData->taxExtra){
					continue;
				}
				const auto dividendForReferrer = static_cast<boost::uint64_t>(amount * (*extraIt));
				LOG_EMPERY_PROMOTION_DEBUG("> Extra bonus: currentReferrerId = ", currentReferrerId,
					", extraGeneration = ", extraIt - extraRatioArray.begin(), ", dividendForReferrer = ", dividendForReferrer);
				transaction.emplace_back(currentReferrerId,
					ItemTransactionElement::OP_ADD, ItemIds::ID_ACCOUNT_BALANCE, dividendForReferrer);
				++extraIt;
			}
		}

		const auto generationDividend = static_cast<boost::uint64_t>(amount * generationRatio);
		unsigned generation = 0;
		for(auto referrerIt = referrers.begin(); (referrerIt != referrers.end()) && (generation < generationCount); ++referrerIt){
			const auto currentReferrerId = referrerIt->first;

			LOG_EMPERY_PROMOTION_DEBUG("> Generation bonus: currentReferrerId = ", currentReferrerId,
				", generationDividend = ", generationDividend);
			transaction.emplace_back(currentReferrerId,
				ItemTransactionElement::OP_ADD, ItemIds::ID_ACCOUNT_BALANCE, generationDividend);
			++generation;
		}

		ItemMap::commitTransaction(transaction.data(), transaction.size(),
			Events::ItemChanged::R_BALANCE_BONUS, accountId.get(), payerId.get(), amount, { });
	}
}

void commitFirstBalanceBonus(){
	PROFILE_ME;
	LOG_EMPERY_PROMOTION_INFO("Committing first balancing...");

	const auto levelRatioTotal = getConfig<double>("balance_level_bonus_ratio_total", 0.70);
	const auto extraRatioArray = Poseidon::explode<double>(',',
	                             getConfig<std::string>("balance_level_bonus_extra_ratio_array", "0.02,0.02,0.01"));
	const auto generationRatio = getConfig<double>("balance_generation_bonus_ratio", 0.10);
	const auto generationCount = getConfig<unsigned>("balance_generation_bonus_count", 10);

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
				reallyAccumulateBalanceBonus(info.accountId, it->accountId, it->accountId, level,
					levelRatioTotal, extraRatioArray, generationRatio, generationCount);
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

	const auto levelRatioTotal = getConfig<double>("balance_level_bonus_ratio_total", 0.70);
	const auto extraRatioArray = Poseidon::explode<double>(',',
	                             getConfig<std::string>("balance_level_bonus_extra_ratio_array", "0.02,0.02,0.01"));
	const auto generationRatio = getConfig<double>("balance_generation_bonus_ratio", 0.10);
	const auto generationCount = getConfig<unsigned>("balance_generation_bonus_count", 10);

	reallyAccumulateBalanceBonus(referrerId, accountId, payerId, amount,
		levelRatioTotal, extraRatioArray, generationRatio, generationCount);
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
