#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/err_account.hpp"
#include "../msg/kill.hpp"
#include "../singletons/account_map.hpp"
#include "../singletons/player_session_map.hpp"
#include "../player_session.hpp"
#include "../account.hpp"

namespace EmperyCenter {

namespace {
	void fill_account_object(Poseidon::JsonObject &object, const boost::shared_ptr<Account> &account){
		PROFILE_ME;

		object[sslit("account_uuid")]      = account->get_account_uuid().str();
		object[sslit("platform_id")]       = account->get_platform_id().get();
		object[sslit("login_name")]        = account->get_login_name();

		object[sslit("referrer_uuid")]     = account->get_referrer_uuid().str();
		object[sslit("promotion_level")]   = account->get_promotion_level();
		object[sslit("nick")]              = account->get_nick();
		object[sslit("activated")]         = account->has_been_activated();
		object[sslit("banned_until")]      = account->get_banned_until();

		Poseidon::JsonObject attribute_object;
		boost::container::flat_map<AccountAttributeId, std::string> attributes;
		account->get_attributes(attributes);
		for(auto it = attributes.begin(); it != attributes.end(); ++it){
			char str[64];
			unsigned len = (unsigned)std::sprintf(str, "%lu", (unsigned long)it->first.get());
			attribute_object[SharedNts(str, len)] = std::move(it->second);
		}
	}
}

ADMIN_SERVLET("account/get", root, session, params){
	const auto account_uuid = AccountUuid(params.at("account_uuid"));

	const auto account = AccountMap::get(account_uuid);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_ACCOUNT) <<account_uuid;
	}

	Poseidon::JsonObject object;
	fill_account_object(object, account);
	root[sslit("account")] = std::move(object);

	return Response();
}

ADMIN_SERVLET("account/get_by_login_name", root, session, params){
	const auto platform_id = boost::lexical_cast<PlatformId>(params.at("platform_id"));
	const auto &login_name = params.at("login_name");

	const auto account = AccountMap::get_by_login_name(platform_id, login_name);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_LOGIN_NAME) <<login_name;
	}

	Poseidon::JsonObject object;
	fill_account_object(object, account);
	root[sslit("account")] = std::move(object);

	return Response();
}

ADMIN_SERVLET("account/insert", root, session, params){
	const auto platform_id = boost::lexical_cast<PlatformId>(params.at("platform_id"));
	const auto &login_name = params.at("login_name");
	const auto referrer_uuid = AccountUuid(params.at("referrer_uuid"));
	const auto promotion_level = boost::lexical_cast<unsigned>(params.at("promotion_level"));

	auto account = AccountMap::get_by_login_name(platform_id, login_name);
	if(account){
		return Response(Msg::ERR_DUPLICATE_PLATFORM_LOGIN_NAME) <<login_name;
	}

	if(referrer_uuid){
		const auto referrer = AccountMap::get(referrer_uuid);
		if(!referrer){
			return Response(Msg::ERR_NO_SUCH_REFERRER_UUID) <<referrer_uuid;
		}
	}

	const auto account_uuid = AccountUuid(Poseidon::Uuid::random());
	const auto utc_now = Poseidon::get_utc_time();
	account = boost::make_shared<Account>(account_uuid, platform_id, login_name, referrer_uuid, promotion_level, utc_now, login_name);
	AccountMap::insert(account, session->get_remote_info().ip.get());

	const auto &login_token = params.get("login_token");
	if(!login_token.empty()){
		const auto login_token_expiry_time = boost::lexical_cast<std::uint64_t>(params.at("login_token_expiry_time"));
		account->set_login_token(login_token, login_token_expiry_time);
	}

	root[sslit("account_uuid")] = account_uuid.str();

	return Response();
}

ADMIN_SERVLET("account/ban", root, session, params){
	const auto account_uuid = AccountUuid(params.at("account_uuid"));
	const auto banned_until = boost::lexical_cast<std::uint64_t>(params.at("banned_until"));

	const auto account = AccountMap::get(account_uuid);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_ACCOUNT) <<account_uuid;
	}

	account->set_banned_until(banned_until);

	const auto utc_now = Poseidon::get_utc_time();
	if(utc_now < banned_until){
		account->set_login_token({ }, 0);

		const auto session = PlayerSessionMap::get(account_uuid);
		if(session){
			session->shutdown(Msg::KILL_OPERATOR_COMMAND, "");
		}
	}

	return Response();
}

}

