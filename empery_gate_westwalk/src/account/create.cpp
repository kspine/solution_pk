#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../msg/err_account.hpp"
#include <poseidon/singletons/event_dispatcher.hpp>
#include "../../../empery_center/src/events/account.hpp"

namespace EmperyGateWestwalk {

ACCOUNT_SERVLET("create", session, params){
	const auto &account_name = params.at("accountName");
	const auto &token       = params.at("token");
	const auto &remarks     = params.get("remarks");

	Poseidon::JsonObject ret;
	if(AccountMap::has(account_name)){
		ret[sslit("errCode")] = (int)Msg::ERR_DUPLICATE_ACCOUNT;
		ret[sslit("msg")] = "Another account with the same name already exists";
		return ret;
	}

	const auto utc_now = Poseidon::get_utc_time();

	const auto platform_id          = get_config<EmperyCenter::PlatformId>("platform_id");
	const auto token_expiry_duration = get_config<boost::uint64_t>("token_expiry_duration", 604800000);

	AccountMap::create(account_name, token, session->get_remote_info().ip.get(), remarks, 0);
	LOG_EMPERY_GATE_WESTWALK_INFO("Created account: account_name = ", account_name, ", token = ", token,
		", remote_info = ", session->get_remote_info(), ", remarks = ", remarks);
	Poseidon::EventDispatcher::sync_raise(
		boost::make_shared<EmperyCenter::Events::AccountSetToken>(
			platform_id, account_name, token, utc_now + token_expiry_duration, session->get_remote_info().ip.get()));

	ret[sslit("errCode")] = (int)Msg::ST_OK;
	ret[sslit("msg")] = "No error";
	return ret;
}

}
