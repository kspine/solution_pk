#include "../precompiled.hpp"
#include "common.hpp"
#include "../mmain.hpp"
#include <poseidon/json.hpp>
#include "../msg/cs_account.hpp"
#include "../msg/sc_account.hpp"
#include "../msg/err_account.hpp"
#include "../singletons/global_status.hpp"
#include "../singletons/bid_record_map.hpp"
#include "../singletons/http_client_daemon.hpp"
#include "../../../empery_promotion/src/msg/err_account.hpp"
#include "../utilities.hpp"

namespace EmperyGoldScramble {

namespace Msg {
	using namespace EmperyPromotion::Msg;
}

namespace {
	std::string g_php_server_host          = "www.paik.im";
	unsigned    g_php_server_port          = 80;
	bool        g_php_server_use_ssl       = false;
	std::string g_php_server_auth          = "";
	std::string g_php_server_path          = "/api/";

	std::string g_promotion_server_host    = "127.0.0.1";
	unsigned    g_promotion_server_port    = 6212;
	bool        g_promotion_server_use_ssl = false;
	std::string g_promotion_server_auth    = "";
	std::string g_promotion_server_path    = "/empery_promotion/account/";

	MODULE_RAII(){
		get_config(g_php_server_host,          "php_session_server_host");
		get_config(g_php_server_port,          "php_session_server_port");
		get_config(g_php_server_use_ssl,       "php_session_server_use_ssl");
		get_config(g_php_server_auth,          "php_session_server_auth_user_pass");
		get_config(g_php_server_path,          "php_session_server_path");

		get_config(g_promotion_server_host,    "promotion_http_server_host");
		get_config(g_promotion_server_port,    "promotion_http_server_port");
		get_config(g_promotion_server_use_ssl, "promotion_http_server_use_ssl");
		get_config(g_promotion_server_auth,    "promotion_http_server_auth_user_pass");
		get_config(g_promotion_server_path,    "promotion_http_server_path");
	}

	void bump_end_time(std::uint64_t utc_now){
		PROFILE_ME;

		const auto game_end_time = GlobalStatus::get(GlobalStatus::SLOT_GAME_END_TIME);
		if(saturated_sub(game_end_time, utc_now) <= 30000){
			const auto gold_coins_in_pot      = GlobalStatus::get(GlobalStatus::SLOT_GOLD_COINS_IN_POT);
			const auto account_balance_in_pot = GlobalStatus::get(GlobalStatus::SLOT_ACCOUNT_BALANCE_IN_POT);
			const auto sum = saturated_add(gold_coins_in_pot, account_balance_in_pot / 100);
			if(sum >= 5000){
				GlobalStatus::fetch_add(GlobalStatus::SLOT_GAME_END_TIME, 2000);
			} else if(sum >= 2000){
				GlobalStatus::fetch_add(GlobalStatus::SLOT_GAME_END_TIME, 5000);
			} else {
				GlobalStatus::fetch_add(GlobalStatus::SLOT_GAME_END_TIME, 10000);
			}
		}
	}
}

PLAYER_SERVLET_RAW(Msg::CS_AccountLogin, session, req){
	const auto old_login_name = PlayerSessionMap::get_login_name(session);
	if(!old_login_name.empty()){
		return Response(Msg::ERR_MULTIPLE_LOGIN);
	}

	std::string login_name;
	{
		Poseidon::OptionalMap params;
		params.set(sslit("sessionId"), req.session_id);
		auto response = HttpClientDaemon::get(g_php_server_host, g_php_server_port, g_php_server_use_ssl, g_php_server_auth,
			g_php_server_path + "getLoginNameFromSessionId", std::move(params));
		if(response.status_code != Poseidon::Http::ST_OK){
			LOG_EMPERY_GOLD_SCRAMBLE_WARNING("PHP server returned an HTTP error: status_code = ", response.status_code);
			return Response(Msg::ST_INTERNAL_ERROR) <<"PHP server returned an HTTP error";
		}
		std::istringstream iss(response.entity.dump());
		auto root = Poseidon::JsonParser::parse_object(iss);
		LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("PHP server response: ", root.dump());
		const bool st = root.at(sslit("st")).get<double>();
		if(!st){
			return Response(Msg::ERR_INVALID_SESSION_ID) <<req.session_id;
		}
		login_name = root.at(sslit("rs")).get<Poseidon::JsonObject>().at(sslit("loginName")).get<std::string>();
	}

	std::string nick;
	std::uint64_t gold_coins = 0, account_balance = 0;
	{
		Poseidon::OptionalMap params;
		params.set(sslit("loginName"), login_name);
		auto response = HttpClientDaemon::get(g_promotion_server_host, g_promotion_server_port, g_promotion_server_use_ssl, g_promotion_server_auth,
			g_promotion_server_path + "queryAccountItems", std::move(params));
		if(response.status_code != Poseidon::Http::ST_OK){
			LOG_EMPERY_GOLD_SCRAMBLE_WARNING("Promotion server returned an HTTP error: status_code = ", response.status_code);
			return Response(Msg::ST_INTERNAL_ERROR) <<"Promotion server returned an HTTP error";
		}
		std::istringstream iss(response.entity.dump());
		auto root = Poseidon::JsonParser::parse_object(iss);
		LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("Promotion server response: ", root.dump());
		const long error_code = root.at(sslit("errorCode")).get<double>();
		if(error_code != 0){
			return Response(Msg::ERR_INVALID_SESSION_ID) <<req.session_id;
		}
		nick = root.at(sslit("nick")).get<std::string>();
		auto &items = root.at(sslit("items")).get<Poseidon::JsonObject>();
		auto it = items.find(sslit("10004"));
		if(it != items.end()){
			gold_coins = it->second.get<double>();
		}
		it = items.find(sslit("10002"));
		if(it != items.end()){
			account_balance = it->second.get<double>();
		}
	}

	PlayerSessionMap::add(login_name, nick, session);
	LOG_EMPERY_GOLD_SCRAMBLE_INFO("Account logged in: session_id = ", req.session_id, ", login_name = ", login_name, ", nick = ", nick);

	session->send(Msg::SC_AccountLoginSuccess(std::move(login_name), std::move(nick)));
	session->send(Msg::SC_AccountAccountBalance(gold_coins, account_balance));
	send_auction_status_to_client(session);
	send_last_log_to_client(session);

	return Response();
}

PLAYER_SERVLET(Msg::CS_AccountBidUsingGoldCoins, login_name, session, /* req */){
	const auto utc_now = Poseidon::get_utc_time();
	const auto game_begin_time = GlobalStatus::get(GlobalStatus::SLOT_GAME_BEGIN_TIME);
	if(utc_now < game_begin_time){
		return Response(Msg::ERR_NO_GAME_IN_PROGRESS) <<game_begin_time;
	}
/*
	std::vector<BidRecordMap::Record> last_player;
	BidRecordMap::get_all(last_player, 1);
	if(!last_player.empty() && (last_player.back().login_name == login_name)){
		return Response(Msg::ERR_DUPLICATE_BID);
	}
*/
	auto nick = PlayerSessionMap::get_nick(session);

	const auto gold_coins_cost   = get_config<std::uint64_t>("bid_gold_coins_cost",   2);
	const auto gold_coins_reward = get_config<std::uint64_t>("bid_gold_coins_reward", 1);

	const auto gold_coins_in_pot      = GlobalStatus::get(GlobalStatus::SLOT_GOLD_COINS_IN_POT);
	const auto account_balance_in_pot = GlobalStatus::get(GlobalStatus::SLOT_ACCOUNT_BALANCE_IN_POT);

	bump_end_time(utc_now); // 在异步请求返回之前增加时间，降低由于网络延迟造成扣了道具但是当前游戏结束的风险。

	Poseidon::OptionalMap params;
	params.set(sslit("loginName"), login_name);
	params.set(sslit("goldCoins"), boost::lexical_cast<std::string>(gold_coins_cost));
	params.set(sslit("accountBalance"), "0");
	params.set(sslit("gameBeginTime"), boost::lexical_cast<std::string>(game_begin_time));
	params.set(sslit("goldCoinsInPot"), boost::lexical_cast<std::string>(gold_coins_in_pot));
	params.set(sslit("accountBalanceInPot"), boost::lexical_cast<std::string>(account_balance_in_pot));
	auto response = HttpClientDaemon::get(g_promotion_server_host, g_promotion_server_port, g_promotion_server_use_ssl, g_promotion_server_auth,
		g_promotion_server_path + "notifyGoldScrambleBid", std::move(params));
	if(response.status_code != Poseidon::Http::ST_OK){
		LOG_EMPERY_GOLD_SCRAMBLE_WARNING("Promotion server returned an HTTP error: status_code = ", response.status_code);
		return Response(Msg::ST_INTERNAL_ERROR) <<"Promotion server returned an HTTP error";
	}
	std::istringstream iss(response.entity.dump());
	auto root = Poseidon::JsonParser::parse_object(iss);
	LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("Promotion server response: ", root.dump());
	const long error_code = root.at(sslit("errorCode")).get<double>();
	if(error_code == Msg::ERR_NO_ENOUGH_ITEMS){
		return Response(Msg::ERR_NO_ENOUGH_GOLD_COINS) <<gold_coins_cost;
	}

	GlobalStatus::fetch_add(GlobalStatus::SLOT_GOLD_COINS_IN_POT, gold_coins_reward);
	BidRecordMap::append(login_name, std::move(nick), gold_coins_cost, 0);
	invalidate_auction_status();

	const auto gold_coins = static_cast<std::uint64_t>(root.at(sslit("goldCoins")).get<double>());
	const auto account_balance = static_cast<std::uint64_t>(root.at(sslit("accountBalance")).get<double>());
	session->send(Msg::SC_AccountAccountBalance(gold_coins, account_balance));

	return Response();
}

PLAYER_SERVLET(Msg::CS_AccountBidUsingAccountBalance, login_name, session, /* req */){
	const auto utc_now = Poseidon::get_utc_time();
	const auto game_begin_time = GlobalStatus::get(GlobalStatus::SLOT_GAME_BEGIN_TIME);
	if(utc_now < game_begin_time){
		return Response(Msg::ERR_NO_GAME_IN_PROGRESS) <<game_begin_time;
	}
/*
	std::vector<BidRecordMap::Record> last_player;
	BidRecordMap::get_all(last_player, 1);
	if(!last_player.empty() && (last_player.back().login_name == login_name)){
		return Response(Msg::ERR_DUPLICATE_BID);
	}
*/
	auto nick = PlayerSessionMap::get_nick(session);

	const auto account_balance_cost   = get_config<std::uint64_t>("bid_account_balance_cost",   200);
	const auto account_balance_reward = get_config<std::uint64_t>("bid_account_balance_reward", 100);

	const auto gold_coins_in_pot      = GlobalStatus::get(GlobalStatus::SLOT_GOLD_COINS_IN_POT);
	const auto account_balance_in_pot = GlobalStatus::get(GlobalStatus::SLOT_ACCOUNT_BALANCE_IN_POT);

	bump_end_time(utc_now); // 在异步请求返回之前增加时间，降低由于网络延迟造成扣了道具但是当前游戏结束的风险。

	Poseidon::OptionalMap params;
	params.set(sslit("loginName"), login_name);
	params.set(sslit("goldCoins"), "0");
	params.set(sslit("accountBalance"), boost::lexical_cast<std::string>(account_balance_cost));
	params.set(sslit("gameBeginTime"), boost::lexical_cast<std::string>(game_begin_time));
	params.set(sslit("goldCoinsInPot"), boost::lexical_cast<std::string>(gold_coins_in_pot));
	params.set(sslit("accountBalanceInPot"), boost::lexical_cast<std::string>(account_balance_in_pot));
	auto response = HttpClientDaemon::get(g_promotion_server_host, g_promotion_server_port, g_promotion_server_use_ssl, g_promotion_server_auth,
		g_promotion_server_path + "notifyGoldScrambleBid", std::move(params));
	if(response.status_code != Poseidon::Http::ST_OK){
		LOG_EMPERY_GOLD_SCRAMBLE_WARNING("Promotion server returned an HTTP error: status_code = ", response.status_code);
		return Response(Msg::ST_INTERNAL_ERROR) <<"Promotion server returned an HTTP error";
	}
	std::istringstream iss(response.entity.dump());
	auto root = Poseidon::JsonParser::parse_object(iss);
	LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("Promotion server response: ", root.dump());
	const long error_code = root.at(sslit("errorCode")).get<double>();
	if(error_code == Msg::ERR_NO_ENOUGH_ITEMS){
		return Response(Msg::ERR_NO_ENOUGH_ACCOUNT_BALANCE) <<account_balance_cost;
	}

	GlobalStatus::fetch_add(GlobalStatus::SLOT_ACCOUNT_BALANCE_IN_POT, account_balance_reward);
	BidRecordMap::append(login_name, std::move(nick), 0, account_balance_cost);
	invalidate_auction_status();

	const auto gold_coins = static_cast<std::uint64_t>(root.at(sslit("goldCoins")).get<double>());
	const auto account_balance = static_cast<std::uint64_t>(root.at(sslit("accountBalance")).get<double>());
	session->send(Msg::SC_AccountAccountBalance(gold_coins, account_balance));

	return Response();
}

}
