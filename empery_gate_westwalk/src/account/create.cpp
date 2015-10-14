#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../msg/err_account.hpp"
#include <poseidon/singletons/event_dispatcher.hpp>
#include "../../../texas_cluster/src/events/account.hpp"

namespace TexasGateWestwalk {

ACCOUNT_SERVLET("create", session, params){
	const AUTO_REF(accountName, params.at("accountName"));
	const AUTO_REF(token, params.at("token"));
	const AUTO_REF(remarks, params.get("remarks"));

	Poseidon::JsonObject ret;
	if(AccountMap::has(accountName)){
		ret[sslit("errCode")] = (int)Msg::ERR_DUPLICATE_ACCOUNT;
		ret[sslit("msg")] = "Another account with the same name already exists";
		return ret;
	}

	const AUTO(platformId, getConfig<TexasCluster::PlatformId>("platform_id"));
	const AUTO(tokenExpiryDuration, getConfig<boost::uint64_t>("token_expiry_duration", 604800000));

	AccountMap::create(accountName, token, session->getRemoteInfo().ip.get(), remarks, 0);
	LOG_TEXAS_GATE_WESTWALK_INFO("Created account: accountName = ", accountName, ", token = ", token,
		", remoteInfo = ", session->getRemoteInfo(), ", remarks = ", remarks);
	Poseidon::EventDispatcher::syncRaise(
		boost::make_shared<TexasCluster::Events::AccountSetToken>(
			platformId, accountName, token, tokenExpiryDuration));

	ret[sslit("errCode")] = (int)Msg::ST_OK;
	ret[sslit("msg")] = "No error";
	return ret;
}

}
