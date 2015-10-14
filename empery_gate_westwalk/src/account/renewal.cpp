#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../msg/err_account.hpp"
#include <poseidon/singletons/event_dispatcher.hpp>
#include "../../../empery_center/src/events/account.hpp"

namespace EmperyGateWestwalk {

ACCOUNT_SERVLET("renewal", /* session */, params){
	const auto &accountName = params.at("accountName");
	const auto &oldToken    = params.at("oldToken");
	const auto &token       = params.at("token");

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
	if(oldToken != info.token){
		LOG_EMPERY_GATE_WESTWALK_DEBUG("Old token is incorrect: expecting ", info.token, ", got ", oldToken);
		ret[sslit("errCode")] = (int)Msg::ERR_OLD_TOKEN_INCORRECT;
		ret[sslit("msg")] = "Old token is incorrect";
		return ret;
	}
	const auto localNow = Poseidon::getLocalTime();
	if(localNow >= info.tokenExpiryTime){
		ret[sslit("errCode")] = (int)Msg::ERR_TOKEN_EXPIRED;
		ret[sslit("msg")] = "Token has expired";
		return ret;
	}
	if((info.bannedUntil != 0) && (localNow < info.bannedUntil)){
		ret[sslit("errCode")] = (int)Msg::ERR_ACCOUNT_BANNED;
		ret[sslit("msg")] = "Account is banned";
		return ret;
	}

	const auto platformId          = getConfig<EmperyCenter::PlatformId>("platform_id");
	const auto tokenExpiryDuration = getConfig<boost::uint64_t>("token_expiry_duration", 604800000);

	LOG_EMPERY_GATE_WESTWALK_INFO("Renewal token: accountName = ", accountName, ", oldToken = ", oldToken, ", token = ", token);
	AccountMap::setToken(accountName, token, localNow + tokenExpiryDuration);
	Poseidon::EventDispatcher::syncRaise(
		boost::make_shared<EmperyCenter::Events::AccountSetToken>(
			platformId, accountName, token, localNow + tokenExpiryDuration));

	ret[sslit("errCode")] = (int)Msg::ST_OK;
	ret[sslit("msg")] = "No error";
	return ret;
}

}
