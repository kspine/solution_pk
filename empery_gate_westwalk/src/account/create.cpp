#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../msg/err_account.hpp"
#include <poseidon/singletons/event_dispatcher.hpp>
#include "../../../empery_center/src/events/account.hpp"

namespace EmperyGateWestwalk {

ACCOUNT_SERVLET("create", session, params){
	const auto &accountName = params.at("accountName");
	const auto &token       = params.at("token");
	const auto &remarks     = params.get("remarks");

	Poseidon::JsonObject ret;
	if(AccountMap::has(accountName)){
		ret[sslit("errCode")] = (int)Msg::ERR_DUPLICATE_ACCOUNT;
		ret[sslit("msg")] = "Another account with the same name already exists";
		return ret;
	}

	const auto localNow = Poseidon::getLocalTime();

	const auto platformId          = getConfig<EmperyCenter::PlatformId>("platform_id");
	const auto tokenExpiryDuration = getConfig<boost::uint64_t>("token_expiry_duration", 604800000);

	AccountMap::create(accountName, token, session->getRemoteInfo().ip.get(), remarks, 0);
	LOG_EMPERY_GATE_WESTWALK_INFO("Created account: accountName = ", accountName, ", token = ", token,
		", remoteInfo = ", session->getRemoteInfo(), ", remarks = ", remarks);
	Poseidon::EventDispatcher::syncRaise(
		boost::make_shared<EmperyCenter::Events::AccountSetToken>(
			platformId, accountName, token, localNow + tokenExpiryDuration, session->getRemoteInfo().ip.get()));

	ret[sslit("errCode")] = (int)Msg::ST_OK;
	ret[sslit("msg")] = "No error";
	return ret;
}

}
