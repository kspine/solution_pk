#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/err_account.hpp"
#include "../singletons/account_map.hpp"
#include "../account.hpp"
#include "../checked_arithmetic.hpp"
#include "../singletons/activation_code_map.hpp"
#include "../activation_code.hpp"

namespace EmperyCenter {

constexpr auto TEST_PLATFORM_ID = PlatformId(7801);

ACCOUNT_SERVLET("test/check_login", root, session, params){
	const auto &login_name = params.at("loginName");
	const auto &password   = params.at("password");
	const auto &token      = params.at("token");

	AccountUuid referrer_uuid;
	if(!password.empty()){
		const auto referrer_account = AccountMap::get_by_login_name(TEST_PLATFORM_ID, password);
		if(!referrer_account){
			return Response(Msg::ERR_NO_SUCH_REFERRER) <<password;
		}
		referrer_uuid = referrer_account->get_account_uuid();
	}

	const auto utc_now = Poseidon::get_utc_time();

	auto account = AccountMap::get_by_login_name(TEST_PLATFORM_ID, login_name);
	if(!account){
		const auto account_uuid = AccountUuid(Poseidon::Uuid::random());
		LOG_EMPERY_CENTER_INFO("Creating new account: account_uuid = ", account_uuid, ", login_name = ", login_name);
		account = boost::make_shared<Account>(account_uuid, TEST_PLATFORM_ID, login_name,
			referrer_uuid, 19, utc_now, login_name);
		AccountMap::insert(account, session->get_remote_info().ip.get());
	}
	if(utc_now < account->get_banned_until()){
		return Response(Msg::ERR_ACCOUNT_BANNED) <<login_name;
	}

	const auto token_expiry_duration = get_config<boost::uint64_t>("account_token_expiry_duration", 0);

	account->set_login_token(token, saturated_add(utc_now, token_expiry_duration));

	root[sslit("hasBeenActivated")]    = account->has_been_activated();
	root[sslit("tokenExpiryDuration")] = token_expiry_duration;

	return Response();
}

ACCOUNT_SERVLET("test/renewal_token", root, session, params){
	const auto &login_name = params.at("loginName");
	const auto &old_token  = params.at("oldToken");
	const auto &token      = params.at("token");

	const auto account = AccountMap::get_by_login_name(TEST_PLATFORM_ID, login_name);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_LOGIN_NAME) <<login_name;
	}
	const auto utc_now = Poseidon::get_utc_time();
	if(utc_now < account->get_banned_until()){
		return Response(Msg::ERR_ACCOUNT_BANNED) <<login_name;
	}
	if(old_token.empty()){
		LOG_EMPERY_CENTER_DEBUG("Empty token");
		return Response(Msg::ERR_INVALID_TOKEN) <<login_name;
	}
	if(old_token != account->get_login_token()){
		LOG_EMPERY_CENTER_DEBUG("Invalid token: expecting ", account->get_login_token(), ", got ", old_token);
		return Response(Msg::ERR_INVALID_TOKEN) <<login_name;
	}

	const auto token_expiry_duration = get_config<boost::uint64_t>("account_token_expiry_duration", 0);

	account->set_login_token(token, saturated_add(utc_now, token_expiry_duration));

	root[sslit("tokenExpiryDuration")] = token_expiry_duration;

	return Response();
}

ACCOUNT_SERVLET("test/activate", root, session, params){
	const auto &login_name = params.at("loginName");
//	const auto &code       = params.at("activationCode");

	const auto account = AccountMap::get_by_login_name(TEST_PLATFORM_ID, login_name);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_LOGIN_NAME) <<login_name;
	}
	if(account->has_been_activated()){
		return Response(Msg::ERR_ACCOUNT_ALREADY_ACTIVATED);
	}
//	const auto activation_code = ActivationCodeMap::get(code);
//	if(!activation_code || activation_code->is_virtually_removed()){
//		return Response(Msg::ERR_ACTIVATION_CODE_DELETED);
//	}

	account->activate();
//	activation_code->set_used_by_account(account->get_account_uuid());

	return Response();
}

}
