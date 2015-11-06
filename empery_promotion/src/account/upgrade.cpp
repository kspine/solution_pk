#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../msg/err_account.hpp"
#include "../data/promotion.hpp"
#include "../utilities.hpp"

namespace EmperyPromotion {

ACCOUNT_SERVLET("upgrade", session, params){
	const auto &payerLoginName = params.at("payerLoginName");
	const auto &loginName = params.at("loginName");
	const auto newLevel = boost::lexical_cast<boost::uint64_t>(params.at("newLevel"));
	const auto &dealPassword = params.at("dealPassword");
	const auto &remarks = params.get("remarks");
	const auto &additionalCardsStr = params.get("additionalCards");

	Poseidon::JsonObject ret;

	auto payerInfo = AccountMap::getByLoginName(payerLoginName);
	if(Poseidon::hasNoneFlagsOf(payerInfo.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_PAYER;
		ret[sslit("errorMessage")] = "Payer is not found";
		return ret;
	}
	if(AccountMap::getPasswordHash(dealPassword) != payerInfo.dealPasswordHash){
		ret[sslit("errorCode")] = (int)Msg::ERR_INVALID_DEAL_PASSWORD;
		ret[sslit("errorMessage")] = "Deal password is incorrect";
		return ret;
	}
	const auto localNow = Poseidon::getUtcTime();
	if((payerInfo.bannedUntil != 0) && (localNow < payerInfo.bannedUntil)){
		ret[sslit("errorCode")] = (int)Msg::ERR_ACCOUNT_BANNED;
		ret[sslit("errorMessage")] = "Payer is banned";
		return ret;
	}

	auto info = AccountMap::getByLoginName(loginName);
	if(Poseidon::hasNoneFlagsOf(info.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
		ret[sslit("errorMessage")] = "Account is not found";
		return ret;
	}
	if((info.bannedUntil != 0) && (localNow < info.bannedUntil)){
		ret[sslit("errorCode")] = (int)Msg::ERR_ACCOUNT_BANNED;
		ret[sslit("errorMessage")] = "Account is banned";
		return ret;
	}

	const auto promotionData = Data::Promotion::get(newLevel);
	if(!promotionData){
		ret[sslit("errorCode")] = (int)Msg::ERR_UNKNOWN_ACCOUNT_LEVEL;
		ret[sslit("errorMessage")] = "Account level is not found";
		return ret;
	}

	boost::uint64_t additionalCards = 0;
	if(!additionalCardsStr.empty()){
		additionalCards = boost::lexical_cast<boost::uint64_t>(additionalCardsStr);
	}

	const auto result = tryUpgradeAccount(info.accountId, payerInfo.accountId, false, promotionData, remarks, additionalCards);
	ret[sslit("balanceToConsume")] = result.second;
	if(!result.first){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_ENOUGH_ACCOUNT_BALANCE;
		ret[sslit("errorMessage")] = "No enough account balance";
		return ret;
	}
	accumulateBalanceBonus(info.accountId, info.accountId, result.second, promotionData->level);

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
