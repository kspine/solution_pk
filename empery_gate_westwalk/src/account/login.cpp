#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../msg/err_account.hpp"
#include <poseidon/singletons/event_dispatcher.hpp>
#include "../../../empery_center/src/events/account.hpp"

namespace EmperyGateWestwalk {

ACCOUNT_SERVLET("login", /* session */, params){
	const auto &accountName = params.at("accountName");
	const auto &password    = params.at("password");
	const auto &token       = params.at("token");

	Poseidon::JsonObject ret;
	auto info = AccountMap::get(accountName);
	if(Poseidon::hasNoneFlagsOf(info.flags, AccountMap::FL_VALID)){
		ret[sslit("errCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
		ret[sslit("msg")] = "The specified account is not found";
		return ret;
	}
	const auto passwordHash = AccountMap::getPasswordHash(password);
	if(Poseidon::hasAnyFlagsOf(info.flags, AccountMap::FL_FROZEN) && (passwordHash != info.disposablePasswordHash)){
		ret[sslit("errCode")] = (int)Msg::ERR_ACCOUNT_FROZEN;
		ret[sslit("msg")] = "The specified account is frozen";
		return ret;
	}
	const auto localNow = Poseidon::getLocalTime();
	if(passwordHash != info.passwordHash){
		if(passwordHash != info.disposablePasswordHash){
			const auto retryCount = AccountMap::decrementPasswordRetryCount(accountName);

			ret[sslit("errCode")] = (int)Msg::ERR_PASSWORD_INCORRECT;
			ret[sslit("msg")] = "Password is incorrect";
			ret[sslit("retryCount")] = retryCount;
			return ret;
		}
		if(localNow >= info.disposablePasswordExpiryTime){
			ret[sslit("errCode")] = (int)Msg::ERR_DISPOSABLE_PASSWD_EXPIRED;
			ret[sslit("msg")] = "Disposable password has expired";
			return ret;
		}
		AccountMap::commitDisposablePassword(accountName);
	}
	if((info.bannedUntil != 0) && (localNow < info.bannedUntil)){
		ret[sslit("errCode")] = (int)Msg::ERR_ACCOUNT_BANNED;
		ret[sslit("msg")] = "Account is banned";
		return ret;
	}

	AccountMap::resetPasswordRetryCount(accountName);

	const auto platformId          = getConfig<EmperyCenter::PlatformId>("platform_id");
	const auto tokenExpiryDuration = getConfig<boost::uint64_t>("token_expiry_duration", 604800000);

	LOG_EMPERY_GATE_WESTWALK_INFO("Account login: accountName = ", accountName, ", token = ", token);
	AccountMap::setToken(accountName, token, localNow + tokenExpiryDuration);
	Poseidon::EventDispatcher::syncRaise(
		boost::make_shared<EmperyCenter::Events::AccountSetToken>(
			platformId, accountName, token, localNow + tokenExpiryDuration));

	ret[sslit("errCode")] = (int)Msg::ST_OK;
	ret[sslit("msg")] = "No error";
	ret[sslit("tokenExpiryDuration")] = tokenExpiryDuration;
	return ret;
}

}
