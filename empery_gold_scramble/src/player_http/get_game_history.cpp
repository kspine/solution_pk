#include "../precompiled.hpp"
#include "common.hpp"
#include "../mysql/game_history.hpp"
#include <poseidon/mysql/object_base.hpp>
#include <poseidon/mysql/utilities.hpp>

namespace EmperyGoldScramble {

namespace MySql {
	namespace {

#define MYSQL_OBJECT_NAME	GameAutoIds
#define MYSQL_OBJECT_FIELDS	\
	FIELD_BIGINT_UNSIGNED	(game_auto_id)
#include <poseidon/mysql/object_generator.hpp>

	}
}

PLAYER_HTTP_SERVLET("getGameHistory", session, params){
	const auto &game_auto_id_str = params.get("gameAutoId");
	const auto &begin = params.get("begin");
	const auto &count = params.get("count");
	const auto &time_begin = params.get("timeBegin");
	const auto &time_end = params.get("timeEnd");
	const auto &login_name = params.get("loginName");
	const auto &nick = params.get("nick");
	const auto &ids_only = params.get("idsOnly");

	Poseidon::JsonObject ret;

	std::ostringstream oss;
	oss <<"SELECT ";
	if(ids_only.empty()){
		oss <<"* ";
	} else {
		oss <<"DISTINCT(`game_auto_id`) ";
	}
	oss <<"FROM `GoldScramble_GameHistory` WHERE 1=1 ";
	if(!game_auto_id_str.empty()){
		auto game_auto_id = boost::lexical_cast<boost::uint64_t>(game_auto_id_str);
		oss <<"AND `game_auto_id` = " <<game_auto_id <<" ";
	}
	if(!time_begin.empty()){
		char str[256];
		Poseidon::format_time(str, sizeof(str), boost::lexical_cast<boost::uint64_t>(time_begin), false);
		oss <<"AND '" <<str <<"' <= `timestamp` ";
		Poseidon::format_time(str, sizeof(str), boost::lexical_cast<boost::uint64_t>(time_end), false);
		oss <<"AND `timestamp` < '" <<str <<"' ";
	}
	if(!login_name.empty()){
		oss <<"AND `login_name` = '" <<Poseidon::MySql::StringEscaper(login_name) <<"' ";
	}
	if(!nick.empty()){
		oss <<"AND `nick` = '" <<Poseidon::MySql::StringEscaper(nick) <<"' ";
	}
	oss <<"ORDER BY `record_auto_id` DESC";
	if(!count.empty()){
		oss <<"LIMIT ";
		if(!begin.empty()){
			auto num_begin = boost::lexical_cast<boost::uint64_t>(begin);
			oss <<num_begin <<", ";
		}
		auto num_count = boost::lexical_cast<boost::uint64_t>(count);
		oss <<num_count;
	}
	if(ids_only.empty()){
		std::vector<boost::shared_ptr<MySql::GoldScramble_GameHistory>> objs;
		MySql::GoldScramble_GameHistory::batch_load(objs, oss.str());

		Poseidon::JsonArray history;
		for(auto it = objs.begin(); it != objs.end(); ++it){
			const auto &obj = *it;

			Poseidon::JsonObject elem;
			elem[sslit("recordAutoId")]      = obj->get_record_auto_id();
			elem[sslit("gameAutoId")]        = obj->get_game_auto_id();
			elem[sslit("timestamp")]         = obj->get_timestamp();
			elem[sslit("loginName")]         = obj->unlocked_get_login_name();
			elem[sslit("nick")]              = obj->unlocked_get_nick();
			elem[sslit("goldCoinsWon")]      = obj->get_gold_coins_won();
			elem[sslit("accountBalanceWon")] = obj->get_account_balance_won();
			history.emplace_back(std::move(elem));
		}
		ret[sslit("history")] = std::move(history);
	} else {
		std::vector<boost::shared_ptr<MySql::GameAutoIds>> objs;
		MySql::GameAutoIds::batch_load(objs, oss.str());

		Poseidon::JsonArray game_auto_ids;
		for(auto it = objs.begin(); it != objs.end(); ++it){
			const auto &obj = *it;

			game_auto_ids.emplace_back(Poseidon::JsonElement(obj->get_game_auto_id()));
		}
		ret[sslit("gameAutoIds")] = std::move(game_auto_ids);
	}

	ret[sslit("errorCode")] = 0;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
