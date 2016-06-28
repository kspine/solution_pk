#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/err_account.hpp"
#include "../msg/err_dungeon.hpp"
#include "../singletons/account_map.hpp"
#include "../singletons/dungeon_box_map.hpp"
#include "../dungeon_box.hpp"

namespace EmperyCenter {

ADMIN_SERVLET("dungeon/get_all", root, session, params){
	const auto account_uuid = AccountUuid(params.at("account_uuid"));

	const auto account = AccountMap::get(account_uuid);
	if(!account_uuid){
		return Response(Msg::ERR_NO_SUCH_ACCOUNT) <<account_uuid;
	}
	AccountMap::require_controller_token(account_uuid);

	const auto dungeon_box = DungeonBoxMap::require(account_uuid);
	dungeon_box->pump_status();

	std::vector<DungeonBox::DungeonInfo> dungeons;
	dungeon_box->get_all(dungeons);

	Poseidon::JsonObject elem_object;
	for(auto it = dungeons.begin(); it != dungeons.end(); ++it){
		Poseidon::JsonObject dungeon_object;
		dungeon_object[sslit("dungeon_id")] = it->dungeon_id.get();
		dungeon_object[sslit("score")]      = it->score;
		char str[64];
		unsigned len = (unsigned)std::sprintf(str, "%lu", (unsigned long)it->dungeon_id.get());
		elem_object[SharedNts(str, len)] = std::move(dungeon_object);
	}
	root[sslit("dungeons")] = std::move(elem_object);

	return Response();
}

ADMIN_SERVLET("dungeon/set", root, session, params){
	const auto account_uuid      = AccountUuid(params.at("account_uuid"));
	const auto dungeon_id        = boost::lexical_cast<DungeonId>(params.at("dungeon_id"));
	const auto &score_str        = params.get("score");
	const auto &entry_count_str  = params.get("entry_count");
	const auto &finish_count_str = params.get("finish_count");

	const auto account = AccountMap::get(account_uuid);
	if(!account_uuid){
		return Response(Msg::ERR_NO_SUCH_ACCOUNT) <<account_uuid;
	}
	AccountMap::require_controller_token(account_uuid);

	const auto dungeon_box = DungeonBoxMap::require(account_uuid);
	dungeon_box->pump_status();

	auto info = dungeon_box->get(dungeon_id);
	if(!score_str.empty()){
		info.score = static_cast<DungeonBox::Score>(boost::lexical_cast<unsigned>(score_str));
	}
	if(!entry_count_str.empty()){
		info.entry_count = boost::lexical_cast<std::uint64_t>(entry_count_str);
	}
	if(!finish_count_str.empty()){
		info.finish_count = boost::lexical_cast<std::uint64_t>(finish_count_str);
	}
	dungeon_box->set(std::move(info));

	return Response();
}

}
