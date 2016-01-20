#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/err_account.hpp"
#include "../msg/kill.hpp"
#include "../singletons/account_map.hpp"
#include "../singletons/player_session_map.hpp"
#include "../player_session.hpp"
#include "../account.hpp"
#include "../account_attribute_ids.hpp"

namespace EmperyCenter {

namespace {
	void fill_account_object(Poseidon::JsonObject &object, const boost::shared_ptr<Account> &account){
		PROFILE_ME;

		object[sslit("account_uuid")]            = account->get_account_uuid().str();
		object[sslit("platform_id")]             = account->get_platform_id().get();
		object[sslit("login_name")]              = account->get_login_name();

		object[sslit("referrer_uuid")]           = account->get_referrer_uuid().str();
		object[sslit("promotion_level")]         = account->get_promotion_level();
		object[sslit("created_time")]            = account->get_created_time();
		object[sslit("nick")]                    = account->get_nick();
		object[sslit("activated")]               = account->has_been_activated();
		object[sslit("banned_until")]            = account->get_banned_until();
		object[sslit("quieted_until")]           = account->get_quieted_until();

		object[sslit("avatar")]                  = account->get_attribute(AccountAttributeIds::ID_AVATAR);
		object[sslit("last_login_time")]         = account->cast_attribute<std::uint64_t>(AccountAttributeIds::ID_LAST_LOGGED_IN_TIME);
		object[sslit("last_logout_time")]        = account->cast_attribute<std::uint64_t>(AccountAttributeIds::ID_LAST_LOGGED_OUT_TIME);
		object[sslit("sequential_sign_in_days")] = account->cast_attribute<std::uint64_t>(AccountAttributeIds::ID_SEQUENTIAL_SIGNED_IN_DAYS);
		object[sslit("auction_center_enabled")]  = account->get_attribute(AccountAttributeIds::ID_AUCTION_CENTER_ENABLED).empty() == false;
		object[sslit("gold_payment_enabled")]    = account->get_attribute(AccountAttributeIds::ID_GOLD_PAYMENT_ENABLED).empty() == false;
		object[sslit("verif_code")]              = account->get_attribute(AccountAttributeIds::ID_VERIF_CODE);
		object[sslit("verif_code_expiry_time")]  = account->cast_attribute<std::uint64_t>(AccountAttributeIds::ID_VERIF_CODE_EXPIRY_TIME);
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

ADMIN_SERVLET("account/set_nick", root, session, params){
	const auto account_uuid = AccountUuid(params.at("account_uuid"));
	const auto &nick = params.at("nick");

	const auto account = AccountMap::get(account_uuid);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_ACCOUNT) <<account_uuid;
	}

	account->set_nick(nick);

	return Response();
}

ADMIN_SERVLET("account/activate", root, session, params){
	const auto account_uuid = AccountUuid(params.at("account_uuid"));

	const auto account = AccountMap::get(account_uuid);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_ACCOUNT) <<account_uuid;
	}

	account->activate();

	return Response();
}

ADMIN_SERVLET("account/get_banned", root, session, params){
	const auto &begin_str = params.get("begin");
	const auto &count_str = params.get("count");

	std::uint64_t begin = 0, count = UINT64_MAX;
	if(!begin_str.empty()){
		begin = boost::lexical_cast<std::uint64_t>(begin_str);
	}
	if(!count_str.empty()){
		count = boost::lexical_cast<std::uint64_t>(count_str);
	}

	std::vector<boost::shared_ptr<Account>> accounts;
	AccountMap::get_banned(accounts);
	root[sslit("count_total")] = accounts.size();

	root[sslit("begin")] = begin;

	Poseidon::JsonArray banned_accounts;
	for(std::uint64_t i = begin; (i - begin < count) && (i < accounts.size()); ++i){
		Poseidon::JsonObject banned_account;
		fill_account_object(banned_account, accounts.at(i));
		banned_accounts.emplace_back(std::move(banned_account));
	}
	root[sslit("banned_accounts")] = std::move(banned_accounts);

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

ADMIN_SERVLET("account/get_quieted", root, session, params){
	const auto &begin_str = params.get("begin");
	const auto &count_str = params.get("count");

	std::uint64_t begin = 0, count = UINT64_MAX;
	if(!begin_str.empty()){
		begin = boost::lexical_cast<std::uint64_t>(begin_str);
	}
	if(!count_str.empty()){
		count = boost::lexical_cast<std::uint64_t>(count_str);
	}

	std::vector<boost::shared_ptr<Account>> accounts;
	AccountMap::get_quieted(accounts);
	root[sslit("count_total")] = accounts.size();

	root[sslit("begin")] = begin;

	Poseidon::JsonArray quieted_accounts;
	for(std::uint64_t i = begin; (i - begin < count) && (i < accounts.size()); ++i){
		Poseidon::JsonObject quieted_account;
		fill_account_object(quieted_account, accounts.at(i));
		quieted_accounts.emplace_back(std::move(quieted_account));
	}
	root[sslit("quieted_accounts")] = std::move(quieted_accounts);

	return Response();
}

ADMIN_SERVLET("account/quiet", root, session, params){
	const auto account_uuid = AccountUuid(params.at("account_uuid"));
	const auto quieted_until = boost::lexical_cast<std::uint64_t>(params.at("quieted_until"));

	const auto account = AccountMap::get(account_uuid);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_ACCOUNT) <<account_uuid;
	}

	account->set_quieted_until(quieted_until);

	return Response();
}

}

