#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../msg/err_account.hpp"
#include "../sms_http_client.hpp"

namespace EmperyGateWestwalk {

ACCOUNT_SERVLET("regain", session, params){
	const auto &account_name = params.at("accountName");

	Poseidon::JsonObject ret;
	auto info = AccountMap::get(account_name);
	if(Poseidon::has_none_flags_of(info.flags, AccountMap::FL_VALID)){
		ret[sslit("errCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
		ret[sslit("msg")] = "The specified account is not found";
		return ret;
	}
	if(Poseidon::has_any_flags_of(info.flags, AccountMap::FL_FROZEN)){
		ret[sslit("errCode")] = (int)Msg::ERR_ACCOUNT_FROZEN;
		ret[sslit("msg")] = "The specified account is frozen";
		return ret;
	}
	const auto local_now = Poseidon::get_local_time();
	if((info.banned_until != 0) && (local_now < info.banned_until)){
		ret[sslit("errCode")] = (int)Msg::ERR_ACCOUNT_BANNED;
		ret[sslit("msg")] = "Account is banned";
		return ret;
	}
	if(local_now < info.password_regain_cooldown_time){
		ret[sslit("errCode")] = (int)Msg::ERR_PASSWORD_RAGAIN_COOLDOWN;
		ret[sslit("msg")] = "Account is in password regain cooldown";
		return ret;
	}

	const auto disposable_password_expiry_duration = get_config<boost::uint64_t>("disposable_password_expiry_duration", 86400000);
	const auto password_regain_cooldown           = get_config<boost::uint64_t>("password_regain_cooldown", 120000);

	auto disposable_password = AccountMap::random_password();
	LOG_EMPERY_GATE_WESTWALK_INFO("Regain: account_name = ", account_name, ", disposable_password = ", disposable_password);
	AccountMap::set_disposable_password(account_name, disposable_password, local_now + disposable_password_expiry_duration);

	const auto client = boost::make_shared<SmsHttpClient>(account_name, std::move(disposable_password));
	client->commit();
	AccountMap::set_password_regain_cooldown_time(account_name, local_now + password_regain_cooldown);

	ret[sslit("errCode")] = (int)Msg::ST_OK;
	ret[sslit("msg")] = "No error";
	ret[sslit("disposablePasswordExpiryDuration")] = disposable_password_expiry_duration;
	ret[sslit("passwordRegainCooldown")] = password_regain_cooldown;
	return ret;
}

}
