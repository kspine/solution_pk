#include "../precompiled.hpp"
#include "common.hpp"
#include "../mysql/game_history.hpp"
#include <poseidon/mysql/utilities.hpp>

namespace EmperyGoldScramble {

namespace MySql {
	namespace {

#define MYSQL_OBJECT_NAME	GameAutoIds
#define MYSQL_OBJECT_FIELDS	\
	FIELD_BIGINT_UNSIGNED	(gameAutoId)
#include <poseidon/mysql/object_generator.hpp>

	}
}

PLAYER_HTTP_SERVLET("getGameHistory", session, params){
	const auto &gameAutoIdStr = params.get("gameAutoId");
	const auto &timeBegin = params.get("timeBegin");
	const auto &timeEnd = params.get("timeEnd");
	const auto &loginName = params.get("loginName");
	const auto &nick = params.get("nick");
	const auto &idsOnly = params.get("idsOnly");

	Poseidon::JsonObject ret;

	std::ostringstream oss;
	oss <<"SELECT ";
	if(idsOnly.empty()){
		oss <<"* ";
	} else {
		oss <<"DISTINCT(`gameAutoId`) ";
	}
	oss <<"FROM `GoldScramble_GameHistory` WHERE 1=1 ";
	if(!gameAutoIdStr.empty()){
		auto gameAutoId = boost::lexical_cast<boost::uint64_t>(gameAutoIdStr);
		oss <<"AND `gameAutoId` = " <<gameAutoId <<" ";
	}
	if(!timeBegin.empty()){
		char str[256];
		Poseidon::formatTime(str, sizeof(str), boost::lexical_cast<boost::uint64_t>(timeBegin), false);
		oss <<"AND '" <<str <<"' <= `timestamp` ";
		Poseidon::formatTime(str, sizeof(str), boost::lexical_cast<boost::uint64_t>(timeEnd), false);
		oss <<"AND `timestamp` < '" <<str <<"' ";
	}
	if(!loginName.empty()){
		oss <<"AND `loginName` = '" <<Poseidon::MySql::StringEscaper(loginName) <<"' ";
	}
	if(!nick.empty()){
		oss <<"AND `nick` = '" <<Poseidon::MySql::StringEscaper(nick) <<"' ";
	}
	oss <<"ORDER BY `recordAutoId` DESC";
	if(idsOnly.empty()){
		std::vector<boost::shared_ptr<MySql::GoldScramble_GameHistory>> objs;
		MySql::GoldScramble_GameHistory::batchLoad(objs, oss.str());

		Poseidon::JsonArray history;
		for(auto it = objs.begin(); it != objs.end(); ++it){
			const auto &obj = *it;

			Poseidon::JsonObject elem;
			elem[sslit("recordAutoId")]      = obj->get_recordAutoId();
			elem[sslit("gameAutoId")]        = obj->get_gameAutoId();
			elem[sslit("timestamp")]         = obj->get_timestamp();
			elem[sslit("loginName")]         = obj->unlockedGet_loginName();
			elem[sslit("nick")]              = obj->unlockedGet_nick();
			elem[sslit("goldCoinsWon")]      = obj->get_goldCoinsWon();
			elem[sslit("accountBalanceWon")] = obj->get_accountBalanceWon();
			history.emplace_back(std::move(elem));
		}
		ret[sslit("history")] = std::move(history);
	} else {
		std::vector<boost::shared_ptr<MySql::GameAutoIds>> objs;
		MySql::GameAutoIds::batchLoad(objs, oss.str());

		Poseidon::JsonArray gameAutoIds;
		for(auto it = objs.begin(); it != objs.end(); ++it){
			const auto &obj = *it;

			gameAutoIds.emplace_back(Poseidon::JsonElement(obj->get_gameAutoId()));
		}
		ret[sslit("gameAutoIds")] = std::move(gameAutoIds);
	}

	ret[sslit("errorCode")] = 0;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
