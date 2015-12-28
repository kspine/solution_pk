#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../msg/err_account.hpp"
#include "../events/account.hpp"

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
	if(AccountMap::get_password_hash(password) != info.password_hash){
		ret[sslit("errorCode")] = (int)Msg::ERR_INVALID_PASSWORD;
		ret[sslit("errorMessage")] = "Password is incorrect";
		return ret;
	}
	if((info.banned_until != 0) && (Poseidon::get_utc_time() < info.banned_until)){
		ret[sslit("errorCode")] = (int)Msg::ERR_ACCOUNT_BANNED;
		ret[sslit("errorMessage")] = "Account is banned";
		return ret;
	}

	Poseidon::async_raise_event(boost::make_shared<Events::AccountLoggedIn>(
		info.account_id, ip));

	ret[sslit("level")] = boost::lexical_cast<std::string>(info.level);

	Poseidon::JsonArray referrers;
	auto referrer_id = info.referrer_id;
	while(referrer_id){
		auto referrer_info = AccountMap::require(referrer_id);

		Poseidon::JsonObject elem;
		elem[sslit("loginName")] = std::move(referrer_info.login_name);
		elem[sslit("level")] = boost::lexical_cast<std::string>(referrer_info.level);
		referrers.emplace_back(std::move(elem));

		referrer_id = referrer_info.referrer_id;
	}
	ret[sslit("referrers")] = std::move(referrers);

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
