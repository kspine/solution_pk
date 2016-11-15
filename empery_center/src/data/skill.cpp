#include "../precompiled.hpp"
#include <poseidon/multi_index_map.hpp>
#include <string.h>
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>
#include "../data_session.hpp"
#include "common.hpp"

namespace EmperyCenter {

namespace {
	const char MONSTER_SKILL_FILE[] = "dungeon_monster_skill";

	MODULE_RAII_PRIORITY(handles, 1000){
		auto csv = Data::sync_load_data(MONSTER_SKILL_FILE);
		
		auto servlet = DataSession::create_servlet(MONSTER_SKILL_FILE, Data::encode_csv_as_json(csv, "id"));
		handles.push(std::move(servlet));
	}
}
}
