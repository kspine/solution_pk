#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../msg/err_account.hpp"
#include <poseidon/singletons/event_dispatcher.hpp>
#include "../../../empery_center/src/events/account.hpp"

namespace EmperyGateWestwalk {

ACCOUNT_SERVLET("login", session, params){
	const auto &account_name = params.at("accountName");
	const auto &password    = params.at("password");
	const auto &token       = params.at("token");

	Poseidon::JsonObject ret;
	auto info = AccountMap::get(account_name);
	if(Poseidon::has_none_flags_of(info.flags, AccountMap::FL_VALID)){
		ret[sslit("errCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
		ret[sslit("msg")] = "The specified account is not found";
		return ret;
	}
	const auto password_hash = AccountMap::get_password_hash(password);
	if(Poseidon::has_any_flags_of(info.flags, AccountMap::FL_FROZEN) && (password_hash != info.disposable_password_hash)){
		ret[sslit("errCode")] = (int)Msg::ERR_ACCOUNT_FROZEN;
		ret[sslit("msg")] = "The specified account is frozen";
		return ret;
	}
	const auto local_now = Poseidon::get_local_time();
	if(password_hash != info.password_hash){
		if(password_hash != info.disposable_password_hash){
			const auto retry_count = AccountMap::decrement_password_retry_count(account_name);

			ret[sslit("errCode")] = (int)Msg::ERR_PASSWORD_INCORRECT;
			ret[sslit("msg")] = "Password is incorrect";
			ret[sslit("retryCount")] = retry_count;
			return ret;
		}
		if(local_now >= info.disposable_password_expiry_time){
			ret[sslit("errCode")] = (int)Msg::ERR_DISPOSABLE_PASSWD_EXPIRED;
			ret[sslit("msg")] = "Disposable password has expired";
			return ret;
		}
		AccountMap::commit_disposable_password(account_name);
	}
	if((info.banned_until != 0) && (local_now < info.banned_until)){
		ret[sslit("errCode")] = (int)Msg::ERR_ACCOUNT_BANNED;
		ret[sslit("msg")] = "Account is banned";
		return ret;
	}

	AccountMap::reset_password_retry_count(account_name);

	const auto platform_id          = get_config<EmperyCenter::PlatformId>("platform_id");
	const auto token_expiry_duration = get_config<boost::uint64_t>("token_expiry_duration", 604800000);

	LOG_EMPERY_GATE_WESTWALK_INFO("Account login: account_name = ", account_name, ", token = ", token);
	AccountMap::set_token(account_name, token, local_now + token_expiry_duration);
	Poseidon::EventDispatcher::sync_raise(
		boost::make_shared<EmperyCenter::Events::AccountSetToken>(
			platform_id, account_name, token, local_now + token_expiry_duration, session->get_remote_info().ip.get()));

	ret[sslit("errCode")] = (int)Msg::ST_OK;
	ret[sslit("msg")] = "No error";
	ret[sslit("tokenExpiryDuration")] = token_expiry_duration;
	return ret;
}

}
