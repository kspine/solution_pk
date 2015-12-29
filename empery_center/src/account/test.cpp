#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/err_account.hpp"
#include "../singletons/account_map.hpp"
#include "../account.hpp"
#include "../checked_arithmetic.hpp"
#include "../singletons/activation_code_map.hpp"
#include "../activation_code.hpp"
#include "../account_attribute_ids.hpp"
#include "../checked_arithmetic.hpp"

namespace EmperyCenter {

constexpr auto PLATFORM_ID = PlatformId(7800);

ACCOUNT_SERVLET("test/check_login", root, session, params){
	const auto &login_name = params.at("loginName");
	const auto &password   = params.at("password");
	const auto &token      = params.at("token");

	AccountUuid referrer_uuid;
	if(!password.empty()){
		const auto referrer_account = AccountMap::get_by_login_name(PLATFORM_ID, password);
		if(!referrer_account){
			return Response(Msg::ERR_NO_SUCH_REFERRER) <<password;
		}
		referrer_uuid = referrer_account->get_account_uuid();
	}

	const auto utc_now = Poseidon::get_utc_time();

	auto account = AccountMap::get_by_login_name(PLATFORM_ID, login_name);
	if(!account){
		const auto account_uuid = AccountUuid(Poseidon::Uuid::random());
		LOG_EMPERY_CENTER_INFO("Creating new account: account_uuid = ", account_uuid, ", login_name = ", login_name);
		account = boost::make_shared<Account>(account_uuid, PLATFORM_ID, login_name,
			referrer_uuid, 19, utc_now, login_name);
		AccountMap::insert(account, session->get_remote_info().ip.get());
	}
	if(utc_now < account->get_banned_until()){
		return Response(Msg::ERR_ACCOUNT_BANNED) <<login_name;
	}

	const auto token_expiry_duration = get_config<std::uint64_t>("account_token_expiry_duration", 0);

	account->set_login_token(token, saturated_add(utc_now, token_expiry_duration));

	root[sslit("hasBeenActivated")]    = account->has_been_activated();
	root[sslit("tokenExpiryDuration")] = token_expiry_duration;

	return Response();
}

ACCOUNT_SERVLET("test/renewal_token", root, session, params){
	const auto &login_name = params.at("loginName");
	const auto &old_token  = params.at("oldToken");
	const auto &token      = params.at("token");

	const auto account = AccountMap::get_by_login_name(PLATFORM_ID, login_name);
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

	const auto token_expiry_duration = get_config<std::uint64_t>("account_token_expiry_duration", 0);

	account->set_login_token(token, saturated_add(utc_now, token_expiry_duration));

	root[sslit("tokenExpiryDuration")] = token_expiry_duration;

	return Response();
}

ACCOUNT_SERVLET("test/regain", root, session, params){
	const auto &login_name = params.at("loginName");

	const auto account = AccountMap::get_by_login_name(PLATFORM_ID, login_name);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_LOGIN_NAME) <<login_name;
	}
	const auto utc_now = Poseidon::get_utc_time();
	const auto old_cooldown = account->cast_attribute<std::uint64_t>(AccountAttributeIds::ID_VERIFICATION_CODE_COOLDOWN);
	if(utc_now < old_cooldown){
		return Response(Msg::ERR_VERIFICATION_CODE_FLOOD);
	}

	char str[64];
	unsigned len = (unsigned)std::sprintf(str, "%06u", (unsigned)(Poseidon::rand64() % 1000000));
	auto verification_code = std::string(str + len - 6, str + len);
	LOG_EMPERY_CENTER_DEBUG("Generated verification code: login_name = ", login_name, ", verification_code = ", verification_code);

 	const auto expiry_duration = get_config<std::uint64_t>("promotion_verification_code_expiry_duration", 900000);
	auto expiry_time = boost::lexical_cast<std::string>(saturated_add(utc_now, expiry_duration));

	const auto cooldown_duration = get_config<std::uint64_t>("promotion_verification_code_cooldown_duration", 120000);
	auto cooldown = boost::lexical_cast<std::string>(saturated_add(utc_now, cooldown_duration));

	boost::container::flat_map<AccountAttributeId, std::string> modifiers;
	modifiers.reserve(3);
	modifiers.emplace(AccountAttributeIds::ID_VERIFICATION_CODE,             std::move(verification_code));
	modifiers.emplace(AccountAttributeIds::ID_VERIFICATION_CODE_EXPIRY_TIME, std::move(expiry_time));
	modifiers.emplace(AccountAttributeIds::ID_VERIFICATION_CODE_COOLDOWN,    std::move(cooldown));
	account->set_attributes(std::move(modifiers));

	// send verification code
	LOG_EMPERY_CENTER_FATAL("Send verification code: login_name = ", login_name, ", verification_code = ", verification_code);

	return Response();
}

ACCOUNT_SERVLET("test/reset_password", root, session, params){
	const auto &login_name        = params.at("loginName");
	const auto &verification_code = params.at("verificationCode");
	const auto &new_password      = params.at("newPassword");

	const auto account = AccountMap::get_by_login_name(PLATFORM_ID, login_name);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_LOGIN_NAME) <<login_name;
	}
	const auto utc_now = Poseidon::get_utc_time();
	const auto expiry_time = account->cast_attribute<std::uint64_t>(AccountAttributeIds::ID_VERIFICATION_CODE_EXPIRY_TIME);
	if(utc_now >= expiry_time){
		return Response(Msg::ERR_VERIFICATION_CODE_EXPIRED);
	}
	const auto &code_expected = account->get_attribute(AccountAttributeIds::ID_VERIFICATION_CODE);
	if(verification_code != code_expected){
		LOG_EMPERY_CENTER_DEBUG("Unexpected verification code: expecting ", code_expected, ", got ", verification_code);
		return Response(Msg::ERR_VERIFICATION_CODE_INCORRECT);
	}

	// set password
	LOG_EMPERY_CENTER_FATAL("Set password: login_name = ", login_name, ", new_password = ", new_password);

	return Response();
}

ACCOUNT_SERVLET("test/activate", root, session, params){
	const auto &login_name = params.at("loginName");
	const auto &code       = params.at("activationCode");

	const auto account = AccountMap::get_by_login_name(PLATFORM_ID, login_name);
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

	// use activation code
	LOG_EMPERY_CENTER_FATAL("Use activation code: login_name = ", login_name, ", code = ", code);

	account->activate();
//	activation_code->set_used_by_account(account->get_account_uuid());

	return Response();
}

}
