#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../msg/err_account.hpp"
#include "../sms_http_client.hpp"

namespace EmperyGateWestwalk {

ACCOUNT_SERVLET("regain", /* session */, params){
	const auto &accountName = params.at("accountName");

	Poseidon::JsonObject ret;
	auto info = AccountMap::get(accountName);
	if(Poseidon::hasNoneFlagsOf(info.flags, AccountMap::FL_VALID)){
		ret[sslit("errCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
		ret[sslit("msg")] = "The specified account is not found";
		return ret;
	}
	if(Poseidon::hasAnyFlagsOf(info.flags, AccountMap::FL_FROZEN)){
		ret[sslit("errCode")] = (int)Msg::ERR_ACCOUNT_FROZEN;
		ret[sslit("msg")] = "The specified account is frozen";
		return ret;
	}
	const auto localNow = Poseidon::getLocalTime();
	if((info.bannedUntil != 0) && (localNow < info.bannedUntil)){
		ret[sslit("errCode")] = (int)Msg::ERR_ACCOUNT_BANNED;
		ret[sslit("msg")] = "Account is banned";
		return ret;
	}
	if(localNow < info.passwordRegainCooldownTime){
		ret[sslit("errCode")] = (int)Msg::ERR_PASSWORD_RAGAIN_COOLDOWN;
		ret[sslit("msg")] = "Account is in password regain cooldown";
		return ret;
	}

	const auto disposablePasswordExpiryDuration = getConfig<boost::uint64_t>("disposable_password_expiry_duration", 86400000);
	const auto passwordRegainCooldown           = getConfig<boost::uint64_t>("password_regain_cooldown", 120000);

	auto disposablePassword = AccountMap::randomPassword();
	LOG_EMPERY_GATE_WESTWALK_INFO("Regain: accountName = ", accountName, ", disposablePassword = ", disposablePassword);
	AccountMap::setDisposablePassword(accountName, disposablePassword, localNow + disposablePasswordExpiryDuration);

	const auto client = boost::make_shared<SmsHttpClient>(accountName, std::move(disposablePassword));
	client->commit();
	AccountMap::setPasswordRegainCooldownTime(accountName, localNow + passwordRegainCooldown);

	ret[sslit("errCode")] = (int)Msg::ST_OK;
	ret[sslit("msg")] = "No error";
	ret[sslit("disposablePasswordExpiryDuration")] = disposablePasswordExpiryDuration;
	ret[sslit("passwordRegainCooldown")] = passwordRegainCooldown;
	return ret;
}

}
