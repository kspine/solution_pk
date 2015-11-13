#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../msg/err_account.hpp"
#include <poseidon/singletons/event_dispatcher.hpp>
#include "../../../empery_center/src/events/account.hpp"

namespace EmperyGateWestwalk {

ACCOUNT_SERVLET("renewal", session, params){
	const auto &account_name = params.at("accountName");
	const auto &old_token    = params.at("oldToken");
	const auto &token       = params.at("token");

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
	if(old_token != info.token){
		LOG_EMPERY_GATE_WESTWALK_DEBUG("Old token is incorrect: expecting ", info.token, ", got ", old_token);
		ret[sslit("errCode")] = (int)Msg::ERR_OLD_TOKEN_INCORRECT;
		ret[sslit("msg")] = "Old token is incorrect";
		return ret;
	}
	const auto local_now = Poseidon::get_local_time();
	if(local_now >= info.token_expiry_time){
		ret[sslit("errCode")] = (int)Msg::ERR_TOKEN_EXPIRED;
		ret[sslit("msg")] = "Token has expired";
		return ret;
	}
	if((info.banned_until != 0) && (local_now < info.banned_until)){
		ret[sslit("errCode")] = (int)Msg::ERR_ACCOUNT_BANNED;
		ret[sslit("msg")] = "Account is banned";
		return ret;
	}

	const auto platform_id          = get_config<EmperyCenter::PlatformId>("platform_id");
	const auto token_expiry_duration = get_config<boost::uint64_t>("token_expiry_duration", 604800000);

	LOG_EMPERY_GATE_WESTWALK_INFO("Renewal token: account_name = ", account_name, ", old_token = ", old_token, ", token = ", token);
	AccountMap::set_token(account_name, token, local_now + token_expiry_duration);
	Poseidon::EventDispatcher::sync_raise(
		boost::make_shared<EmperyCenter::Events::AccountSetToken>(
			platform_id, account_name, token, local_now + token_expiry_duration, session->get_remote_info().ip.get()));

	ret[sslit("errCode")] = (int)Msg::ST_OK;
	ret[sslit("msg")] = "No error";
	return ret;
}

}
