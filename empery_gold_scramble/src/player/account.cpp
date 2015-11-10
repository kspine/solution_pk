#include "../precompiled.hpp"
#include "common.hpp"
#include <poseidon/json.hpp>
#include "../msg/cs_account.hpp"
#include "../msg/sc_account.hpp"
#include "../msg/err_account.hpp"
#include "../singletons/global_status.hpp"
#include "../singletons/bid_record_map.hpp"
#include "../singletons/http_client_daemon.hpp"
#include "../../../empery_promotion/src/msg/err_account.hpp"
#include "../utilities.hpp"
#include "../checked_arithmetic.hpp"

namespace EmperyGoldScramble {

namespace Msg {
	using namespace EmperyPromotion::Msg;
}

namespace {
	std::string g_phpServerHost         = "www.paik.im";
	unsigned    g_phpServerPort         = 80;
	bool        g_phpServerUseSsl       = false;
	std::string g_phpServerAuth         = "";
	std::string g_phpServerPath         = "/api/";

	std::string g_promotionServerHost   = "127.0.0.1";
	unsigned    g_promotionServerPort   = 6212;
	bool        g_promotionServerUseSsl = false;
	std::string g_promotionServerAuth   = "";
	std::string g_promotionServerPath   = "/empery_promotion/account/";

	MODULE_RAII(){
		getConfig(g_phpServerHost,         "php_session_server_host");
		getConfig(g_phpServerPort,         "php_session_server_port");
		getConfig(g_phpServerUseSsl,       "php_session_server_use_ssl");
		getConfig(g_phpServerAuth,         "php_session_server_auth_user_pass");
		getConfig(g_phpServerPath,         "php_session_server_path");

		getConfig(g_promotionServerHost,   "promotion_http_server_host");
		getConfig(g_promotionServerPort,   "promotion_http_server_port");
		getConfig(g_promotionServerUseSsl, "promotion_http_server_use_ssl");
		getConfig(g_promotionServerAuth,   "promotion_http_server_auth_user_pass");
		getConfig(g_promotionServerPath,   "promotion_http_server_path");
	}

	void bumpEndTime(boost::uint64_t utcNow){
		PROFILE_ME;

		const auto gameEndTime = GlobalStatus::get(GlobalStatus::SLOT_GAME_END_TIME);
		if(saturatedSub(gameEndTime, utcNow) <= 30000){
			const auto goldCoinsInPot      = GlobalStatus::get(GlobalStatus::SLOT_GOLD_COINS_IN_POT);
			const auto accountBalanceInPot = GlobalStatus::get(GlobalStatus::SLOT_ACCOUNT_BALANCE_IN_POT);
			const auto sum = saturatedAdd(goldCoinsInPot, accountBalanceInPot / 100);
			if(sum > 10000){
				GlobalStatus::fetchAdd(GlobalStatus::SLOT_GAME_END_TIME, 1000);
			} else if(sum > 5000){
				GlobalStatus::fetchAdd(GlobalStatus::SLOT_GAME_END_TIME, 2000);
			} else {
				GlobalStatus::fetchAdd(GlobalStatus::SLOT_GAME_END_TIME, 10000);
			}
		}
	}
}

PLAYER_SERVLET_RAW(Msg::CS_AccountLogin, session, req){
	const auto oldLoginName = PlayerSessionMap::getLoginName(session);
	if(!oldLoginName.empty()){
		return Response(Msg::ERR_MULTIPLE_LOGIN);
	}

	std::string loginName;
	{
/*		Poseidon::OptionalMap params;
		params.set(sslit("sessionId"), req.sessionId);
		auto response = HttpClientDaemon::get(g_phpServerHost, g_phpServerPort, g_phpServerUseSsl, g_phpServerAuth,
			g_phpServerPath + "getLoginNameFromSessionId", std::move(params));
		if(response.statusCode != Poseidon::Http::ST_OK){
			LOG_EMPERY_GOLD_SCRAMBLE_WARNING("PHP server returned an HTTP error: statusCode = ", response.statusCode);
			return Response(Msg::ST_INTERNAL_ERROR) <<"PHP server returned an HTTP error";
		}
		std::istringstream iss(response.entity.dump());
		auto root = Poseidon::JsonParser::parseObject(iss);
		LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("PHP server response: ", root.dump());
		const bool st = root.at(sslit("st")).get<double>();
		if(!st){
			return Response(Msg::ERR_INVALID_SESSION_ID) <<req.sessionId;
		}
		loginName = root.at(sslit("rs")).get<std::string>();
*/		loginName = req.sessionId;
	}

	std::string nick;
	boost::uint64_t goldCoins = 0, accountBalance = 0;
	{
		Poseidon::OptionalMap params;
		params.set(sslit("loginName"), loginName);
		auto response = HttpClientDaemon::get(g_promotionServerHost, g_promotionServerPort, g_promotionServerUseSsl, g_promotionServerAuth,
			g_promotionServerPath + "queryAccountItems", std::move(params));
		if(response.statusCode != Poseidon::Http::ST_OK){
			LOG_EMPERY_GOLD_SCRAMBLE_WARNING("Promotion server returned an HTTP error: statusCode = ", response.statusCode);
			return Response(Msg::ST_INTERNAL_ERROR) <<"Promotion server returned an HTTP error";
		}
		std::istringstream iss(response.entity.dump());
		auto root = Poseidon::JsonParser::parseObject(iss);
		LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("Promotion server response: ", root.dump());
		const long errorCode = root.at(sslit("errorCode")).get<double>();
		if(errorCode != 0){
			return Response(Msg::ERR_INVALID_SESSION_ID) <<req.sessionId;
		}
		nick = root.at(sslit("nick")).get<std::string>();
		auto &items = root.at(sslit("items")).get<Poseidon::JsonObject>();
		auto it = items.find(sslit("10004"));
		if(it != items.end()){
			goldCoins = it->second.get<double>();
		}
		it = items.find(sslit("10002"));
		if(it != items.end()){
			accountBalance = it->second.get<double>();
		}
	}

	PlayerSessionMap::add(loginName, nick, session);
	LOG_EMPERY_GOLD_SCRAMBLE_INFO("Account logged in: sessionId = ", req.sessionId, ", loginName = ", loginName, ", nick = ", nick);

	session->send(Msg::SC_AccountLoginSuccess(std::move(loginName), std::move(nick)));
	session->send(Msg::SC_AccountAccountBalance(goldCoins, accountBalance));
	sendAuctionStatusToClient(session);

	return Response();
}

PLAYER_SERVLET(Msg::CS_AccountBidUsingGoldCoins, loginName, session, /* req */){
	const auto utcNow = Poseidon::getUtcTime();
	const auto gameBeginTime = GlobalStatus::get(GlobalStatus::SLOT_GAME_BEGIN_TIME);
	if(utcNow < gameBeginTime){
		return Response(Msg::ERR_NO_GAME_IN_PROGRESS) <<gameBeginTime;
	}
	auto nick = PlayerSessionMap::getNick(session);

	const auto goldCoinsCost   = getConfig<boost::uint64_t>("bid_gold_coins_cost",   2);
	const auto goldCoinsReward = getConfig<boost::uint64_t>("bid_gold_coins_reward", 1);

	const auto goldCoinsInPot      = GlobalStatus::get(GlobalStatus::SLOT_GOLD_COINS_IN_POT);
	const auto accountBalanceInPot = GlobalStatus::get(GlobalStatus::SLOT_ACCOUNT_BALANCE_IN_POT);

	Poseidon::OptionalMap params;
	params.set(sslit("loginName"), loginName);
	params.set(sslit("goldCoins"), boost::lexical_cast<std::string>(goldCoinsCost));
	params.set(sslit("accountBalance"), "0");
	params.set(sslit("gameBeginTime"), boost::lexical_cast<std::string>(gameBeginTime));
	params.set(sslit("goldCoinsInPot"), boost::lexical_cast<std::string>(goldCoinsInPot));
	params.set(sslit("accountBalanceInPot"), boost::lexical_cast<std::string>(accountBalanceInPot));
	auto response = HttpClientDaemon::get(g_promotionServerHost, g_promotionServerPort, g_promotionServerUseSsl, g_promotionServerAuth,
		g_promotionServerPath + "notifyGoldScrambleBid", std::move(params));
	if(response.statusCode != Poseidon::Http::ST_OK){
		LOG_EMPERY_GOLD_SCRAMBLE_WARNING("Promotion server returned an HTTP error: statusCode = ", response.statusCode);
		return Response(Msg::ST_INTERNAL_ERROR) <<"Promotion server returned an HTTP error";
	}
	std::istringstream iss(response.entity.dump());
	auto root = Poseidon::JsonParser::parseObject(iss);
	LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("Promotion server response: ", root.dump());
	const long errorCode = root.at(sslit("errorCode")).get<double>();
	if(errorCode == Msg::ERR_NO_ENOUGH_ITEMS){
		return Response(Msg::ERR_NO_ENOUGH_GOLD_COINS) <<goldCoinsCost;
	}

	GlobalStatus::fetchAdd(GlobalStatus::SLOT_GOLD_COINS_IN_POT, goldCoinsReward);
	bumpEndTime(utcNow);
	BidRecordMap::append(loginName, std::move(nick), goldCoinsReward, 0);
	invalidateAuctionStatus();

	const auto goldCoins = static_cast<boost::uint64_t>(root.at(sslit("goldCoins")).get<double>());
	const auto accountBalance = static_cast<boost::uint64_t>(root.at(sslit("accountBalance")).get<double>());
	session->send(Msg::SC_AccountAccountBalance(goldCoins, accountBalance));

	return Response();
}

PLAYER_SERVLET(Msg::CS_AccountBidUsingAccountBalance, loginName, session, /* req */){
	const auto utcNow = Poseidon::getUtcTime();
	const auto gameBeginTime = GlobalStatus::get(GlobalStatus::SLOT_GAME_BEGIN_TIME);
	if(utcNow < gameBeginTime){
		return Response(Msg::ERR_NO_GAME_IN_PROGRESS) <<gameBeginTime;
	}
	auto nick = PlayerSessionMap::getNick(session);

	const auto accountBalanceCost   = getConfig<boost::uint64_t>("bid_account_balance_cost",   200);
	const auto accountBalanceReward = getConfig<boost::uint64_t>("bid_account_balance_reward", 100);

	const auto goldCoinsInPot      = GlobalStatus::get(GlobalStatus::SLOT_GOLD_COINS_IN_POT);
	const auto accountBalanceInPot = GlobalStatus::get(GlobalStatus::SLOT_ACCOUNT_BALANCE_IN_POT);

	Poseidon::OptionalMap params;
	params.set(sslit("loginName"), loginName);
	params.set(sslit("goldCoins"), "0");
	params.set(sslit("accountBalance"), boost::lexical_cast<std::string>(accountBalanceCost));
	params.set(sslit("gameBeginTime"), boost::lexical_cast<std::string>(gameBeginTime));
	params.set(sslit("goldCoinsInPot"), boost::lexical_cast<std::string>(goldCoinsInPot));
	params.set(sslit("accountBalanceInPot"), boost::lexical_cast<std::string>(accountBalanceInPot));
	auto response = HttpClientDaemon::get(g_promotionServerHost, g_promotionServerPort, g_promotionServerUseSsl, g_promotionServerAuth,
		g_promotionServerPath + "notifyGoldScrambleBid", std::move(params));
	if(response.statusCode != Poseidon::Http::ST_OK){
		LOG_EMPERY_GOLD_SCRAMBLE_WARNING("Promotion server returned an HTTP error: statusCode = ", response.statusCode);
		return Response(Msg::ST_INTERNAL_ERROR) <<"Promotion server returned an HTTP error";
	}
	std::istringstream iss(response.entity.dump());
	auto root = Poseidon::JsonParser::parseObject(iss);
	LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("Promotion server response: ", root.dump());
	const long errorCode = root.at(sslit("errorCode")).get<double>();
	if(errorCode == Msg::ERR_NO_ENOUGH_ITEMS){
		return Response(Msg::ERR_NO_ENOUGH_ACCOUNT_BALANCE) <<accountBalanceCost;
	}

	GlobalStatus::fetchAdd(GlobalStatus::SLOT_ACCOUNT_BALANCE_IN_POT, accountBalanceReward);
	bumpEndTime(utcNow);
	BidRecordMap::append(loginName, std::move(nick), accountBalanceReward, 0);
	invalidateAuctionStatus();

	const auto goldCoins = static_cast<boost::uint64_t>(root.at(sslit("goldCoins")).get<double>());
	const auto accountBalance = static_cast<boost::uint64_t>(root.at(sslit("accountBalance")).get<double>());
	session->send(Msg::SC_AccountAccountBalance(goldCoins, accountBalance));

	return Response();
}

}
