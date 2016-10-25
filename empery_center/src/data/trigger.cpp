#include "../precompiled.hpp"
#include <poseidon/multi_index_map.hpp>
#include <string.h>
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>
#include "../data_session.hpp"
#include "common.hpp"

namespace EmperyCenter {

namespace {
	const auto MAX_DUNGE_TRIGGER_COUNT = 27;
	const std::string  DUNGEON_TRIGGER_FILES[MAX_DUNGE_TRIGGER_COUNT] = {
		"dungeon_trigger_0_1","dungeon_trigger_0_2","dungeon_trigger_0_3","dungeon_trigger_0_4","dungeon_trigger_0_5","dungeon_trigger_0_6",
		"dungeon_trigger_1_1","dungeon_trigger_1_2","dungeon_trigger_1_3","dungeon_trigger_1_4","dungeon_trigger_1_5","dungeon_trigger_1_6",
		"dungeon_trigger_2_1","dungeon_trigger_2_2","dungeon_trigger_2_3","dungeon_trigger_2_4","dungeon_trigger_2_5","dungeon_trigger_2_6",
		"dungeon_trigger_3_1","dungeon_trigger_3_2","dungeon_trigger_3_3","dungeon_trigger_3_4","dungeon_trigger_3_5","dungeon_trigger_3_6",
		"dungeon_trigger_4_1","dungeon_trigger_4_2","dungeon_trigger_4_3"};

	MODULE_RAII_PRIORITY(handles, 1000){
		for(auto i = 0; i < MAX_DUNGE_TRIGGER_COUNT;i++){
			auto csv = Data::sync_load_data(DUNGEON_TRIGGER_FILES[i].c_str());
			auto servlet = DataSession::create_servlet(DUNGEON_TRIGGER_FILES[i].c_str(), Data::encode_csv_as_json(csv, "id"));
			handles.push(std::move(servlet));
		}
	}
}

}
