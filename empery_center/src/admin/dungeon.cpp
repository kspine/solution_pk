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
	if(!account){
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
		dungeon_object[sslit("dungeon_type_id")] = it->dungeon_type_id.get();
		dungeon_object[sslit("entry_count")]     = it->entry_count;
		dungeon_object[sslit("finish_count")]    = it->finish_count;
		Poseidon::JsonArray tasks_finished;
		for(auto tit = it->tasks_finished.begin(); tit != it->tasks_finished.end(); ++tit){
			tasks_finished.emplace_back(tit->get());
		}
		dungeon_object[sslit("tasks_finished")] = std::move(tasks_finished);
		char str[64];
		unsigned len = (unsigned)std::sprintf(str, "%lu", (unsigned long)it->dungeon_type_id.get());
		elem_object[SharedNts(str, len)] = std::move(dungeon_object);
	}
	root[sslit("dungeons")] = std::move(elem_object);

	return Response();
}

ADMIN_SERVLET("dungeon/set", root, session, params){
	const auto account_uuid        = AccountUuid(params.at("account_uuid"));
	const auto dungeon_type_id     = boost::lexical_cast<DungeonTypeId>(params.at("dungeon_type_id"));
	const auto &entry_count_str    = params.get("entry_count");
	const auto &finish_count_str   = params.get("finish_count");
	const auto &tasks_finished_str = params.get("tasks_finished");

	const auto account = AccountMap::get(account_uuid);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_ACCOUNT) <<account_uuid;
	}
	AccountMap::require_controller_token(account_uuid);

	const auto dungeon_box = DungeonBoxMap::require(account_uuid);
	dungeon_box->pump_status();

	auto info = dungeon_box->get(dungeon_type_id);
	if(!entry_count_str.empty()){
		info.entry_count = boost::lexical_cast<std::uint64_t>(entry_count_str);
	}
	if(!finish_count_str.empty()){
		info.finish_count = boost::lexical_cast<std::uint64_t>(finish_count_str);
	}
	if(!tasks_finished_str.empty()){
		std::istringstream iss(tasks_finished_str);
		const auto tasks_finished = Poseidon::JsonParser::parse_array(iss);
		info.tasks_finished.reserve(tasks_finished.size());
		for(auto it = tasks_finished.begin(); it != tasks_finished.end(); ++it){
			info.tasks_finished.insert(DungeonTaskId(it->get<double>()));
		}
	}
	dungeon_box->set(std::move(info));

	return Response();
}

}
