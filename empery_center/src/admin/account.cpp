#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/err_account.hpp"
#include "../msg/kill.hpp"
#include "../singletons/account_map.hpp"
#include "../singletons/player_session_map.hpp"
#include "../player_session.hpp"
#include "../account.hpp"

namespace EmperyCenter {

ADMIN_SERVLET("account/create", root, session, params){
	const auto platform_id = boost::lexical_cast<PlatformId>(params.at("platform_id"));
	const auto &login_name = params.at("login_name");
	const auto referrer_uuid = AccountUuid(params.at("referrer_uuid"));

	auto account = AccountMap::get_by_login_name(platform_id, login_name);
	if(account){
		return Response(Msg::ERR_DUPLICATE_PLATFORM_LOGIN_NAME) <<login_name;
	}

	if(referrer_uuid){
		const auto referrer = AccountMap::get(referrer_uuid);
		if(!referrer){
			return Response(Msg::ERR_NO_SUCH_REFERRER) <<referrer_uuid;
		}
	}

	const auto account_uuid = AccountUuid(Poseidon::Uuid::random());
	const auto utc_now = Poseidon::get_utc_time();
	account = boost::make_shared<Account>(account_uuid, platform_id, login_name, referrer_uuid, utc_now, login_name, 0);
	AccountMap::insert(account, session->get_remote_info().ip.get());

	const auto &login_token = params.get("login_token");
	if(!login_token.empty()){
		const auto login_token_expiry_time = boost::lexical_cast<boost::uint64_t>(params.at("login_token_expiry_time"));
		account->set_login_token(login_token, login_token_expiry_time);
	}

	root[sslit("account_uuid")] = account_uuid.str();

	return Response();
}

ADMIN_SERVLET("account/set_token", root, session, params){
	const auto account_uuid = AccountUuid(params.at("account_uuid"));
	const auto &login_token = params.at("login_token");
	const auto login_token_expiry_time = boost::lexical_cast<boost::uint64_t>(params.at("login_token_expiry_time"));

	const auto account = AccountMap::get(account_uuid);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_ACCOUNT) <<account_uuid;
	}

	account->set_login_token(login_token, login_token_expiry_time);

	return Response();
}

ADMIN_SERVLET("account/ban", root, session, params){
	const auto account_uuid = AccountUuid(params.at("account_uuid"));
	const auto banned_until = boost::lexical_cast<boost::uint64_t>(params.at("banned_until"));

	const auto account = AccountMap::get(account_uuid);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_ACCOUNT) <<account_uuid;
	}

	account->set_banned_until(banned_until);

	const auto utc_now = Poseidon::get_utc_time();
	if(utc_now < banned_until){
		const auto session = PlayerSessionMap::get(account_uuid);
		if(session){
			session->shutdown(Msg::KILL_OPERATOR_COMMAND, "");
		}
	}

	return Response();
}

}

