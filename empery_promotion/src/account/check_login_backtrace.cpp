#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../msg/err_account.hpp"
#include "../events/account.hpp"
#include "../singletons/item_map.hpp"
#include "../item_ids.hpp"

namespace EmperyPromotion {

ACCOUNT_SERVLET("checkLoginBacktrace", session, params){
	const auto &login_name = params.at("loginName");
	const auto &password = params.at("password");
	const auto &ip = params.get("ip");

	Poseidon::JsonObject ret;
	auto info = AccountMap::get_by_login_name(login_name);
	if(Poseidon::has_none_flags_of(info.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
		ret[sslit("errorMessage")] = "Account is not found";
		return ret;
	}

	int error_code = (int)Msg::ST_OK;
	std::string error_message;
	if(AccountMap::get_password_hash(password) != info.password_hash){
		error_code = (int)Msg::ERR_INVALID_PASSWORD;
		error_message = "Password is incorrect";
	} else if((info.banned_until != 0) && (Poseidon::get_utc_time() < info.banned_until)){
		error_code = (int)Msg::ERR_ACCOUNT_BANNED;
		error_message = "Account is banned";
	} else {
		error_code = (int)Msg::ST_OK;
		error_message = "No error";
	}

	Poseidon::async_raise_event(boost::make_shared<Events::AccountLoggedIn>(
		info.account_id, ip));

	ret[sslit("level")] = boost::lexical_cast<std::string>(info.level);
	ret[sslit("nick")] = std::move(info.nick);
	ret[sslit("isAuctionCenterEnabled")] = AccountMap::cast_attribute<bool>(info.account_id, AccountMap::ATTR_AUCTION_CENTER_ENABLED);
	ret[sslit("hasAccelerationCards")] = ItemMap::get_count(info.account_id, ItemIds::ID_ACCELERATION_CARDS) != 0;

	Poseidon::JsonArray referrers;
	auto referrer_id = info.referrer_id;
	while(referrer_id){
		auto referrer_info = AccountMap::require(referrer_id);

		Poseidon::JsonObject elem;
		elem[sslit("loginName")] = std::move(referrer_info.login_name);
		elem[sslit("level")] = boost::lexical_cast<std::string>(referrer_info.level);
		elem[sslit("nick")] = std::move(referrer_info.nick);
		elem[sslit("isAuctionCenterEnabled")] = AccountMap::cast_attribute<bool>(referrer_info.account_id, AccountMap::ATTR_AUCTION_CENTER_ENABLED);
		referrers.emplace_back(std::move(elem));

		referrer_id = referrer_info.referrer_id;
	}
	ret[sslit("referrers")] = std::move(referrers);

	ret[sslit("errorCode")] = error_code;
	ret[sslit("errorMessage")] = std::move(error_message);
	return ret;
}

}
