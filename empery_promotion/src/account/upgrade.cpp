#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../msg/err_account.hpp"
#include "../data/promotion.hpp"
#include "../utilities.hpp"

namespace EmperyPromotion {

ACCOUNT_SERVLET("upgrade", session, params){
	const auto &payer_login_name = params.at("payerLoginName");
	const auto &login_name = params.at("loginName");
	const auto new_level = boost::lexical_cast<std::uint64_t>(params.at("newLevel"));
	const auto &deal_password = params.at("dealPassword");
	const auto &remarks = params.get("remarks");
	const auto &additional_cards_str = params.get("additionalCards");

	Poseidon::JsonObject ret;

	auto payer_info = AccountMap::get_by_login_name(payer_login_name);
	if(Poseidon::has_none_flags_of(payer_info.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_PAYER;
		ret[sslit("errorMessage")] = "Payer is not found";
		return ret;
	}
	if(AccountMap::get_password_hash(deal_password) != payer_info.deal_password_hash){
		ret[sslit("errorCode")] = (int)Msg::ERR_INVALID_DEAL_PASSWORD;
		ret[sslit("errorMessage")] = "Deal password is incorrect";
		return ret;
	}
	const auto utc_now = Poseidon::get_utc_time();
	if((payer_info.banned_until != 0) && (utc_now < payer_info.banned_until)){
		ret[sslit("errorCode")] = (int)Msg::ERR_ACCOUNT_BANNED;
		ret[sslit("errorMessage")] = "Payer is banned";
		return ret;
	}

	auto info = AccountMap::get_by_login_name(login_name);
	if(Poseidon::has_none_flags_of(info.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
		ret[sslit("errorMessage")] = "Account is not found";
		return ret;
	}
	if((info.banned_until != 0) && (utc_now < info.banned_until)){
		ret[sslit("errorCode")] = (int)Msg::ERR_ACCOUNT_BANNED;
		ret[sslit("errorMessage")] = "Account is banned";
		return ret;
	}

	const auto promotion_data = Data::Promotion::get(new_level);
	if(!promotion_data){
		ret[sslit("errorCode")] = (int)Msg::ERR_UNKNOWN_ACCOUNT_LEVEL;
		ret[sslit("errorMessage")] = "Account level is not found";
		return ret;
	}

	std::uint64_t additional_cards = 0;
	if(!additional_cards_str.empty()){
		additional_cards = boost::lexical_cast<std::uint64_t>(additional_cards_str);
	}

	const auto result = try_upgrade_account(info.account_id, payer_info.account_id, false, promotion_data, remarks, additional_cards);
	ret[sslit("balanceToConsume")] = result.second;
	if(!result.first){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_ENOUGH_ACCOUNT_BALANCE;
		ret[sslit("errorMessage")] = "No enough account balance";
		return ret;
	}
	accumulate_balance_bonus(info.account_id, info.account_id, result.second, promotion_data->level);
	check_auto_upgradeable(info.referrer_id);

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
