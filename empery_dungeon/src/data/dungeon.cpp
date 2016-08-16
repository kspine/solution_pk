#include "../precompiled.hpp"
#include "dungeon.hpp"
#include <poseidon/multi_index_map.hpp>
#include <string.h>
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>

namespace EmperyDungeon {

namespace {
	MULTI_INDEX_MAP(DungeonContainer, Data::Dungeon,
		UNIQUE_MEMBER_INDEX(dungeon_type_id)
	)
	boost::weak_ptr<const DungeonContainer> g_dungeon_container;
	const char DUNGEON_FILE[] = "dungeon";

	MODULE_RAII_PRIORITY(handles, 1000){
		auto csv = Data::sync_load_data(DUNGEON_FILE);
		const auto dungeon_container = boost::make_shared<DungeonContainer>();
		while(csv.fetch_row()){
			Data::Dungeon elem = { };

			csv.get(elem.dungeon_type_id,              "dungeon_id");
			csv.get(elem.dungeon_map,                  "dungeon_map");
			csv.get(elem.dungeon_trigger,              "dungeon_trigger");
			if(!dungeon_container->insert(std::move(elem)).second){
				LOG_EMPERY_DUNGEON_ERROR("Duplicate Dungeon: dungeon_type_id = ", elem.dungeon_type_id);
				DEBUG_THROW(Exception, sslit("Duplicate Dungeon"));
			}
		}
		g_dungeon_container = dungeon_container;
		handles.push(dungeon_container);
	}
}

namespace Data {
	boost::shared_ptr<const Dungeon> Dungeon::get(DungeonTypeId dungeon_type_id){
		PROFILE_ME;

		const auto dungeon_container = g_dungeon_container.lock();
		if(!dungeon_container){
			LOG_EMPERY_DUNGEON_WARNING("DungeonContainer has not been loaded.");
			return { };
		}

		const auto it = dungeon_container->find<0>(dungeon_type_id);
		if(it == dungeon_container->end<0>()){
			LOG_EMPERY_DUNGEON_TRACE("Dungeon not found: dungeon_type_id = ", dungeon_type_id);
			return { };
		}
		return boost::shared_ptr<const Dungeon>(dungeon_container, &*it);
	}
	boost::shared_ptr<const Dungeon> Dungeon::require(DungeonTypeId dungeon_type_id){
		PROFILE_ME;

		auto ret = get(dungeon_type_id);
		if(!ret){
			LOG_EMPERY_DUNGEON_WARNING("Dungeon not found: dungeon_type_id = ", dungeon_type_id);
			DEBUG_THROW(Exception, sslit("Dungeon not found"));
		}
		return ret;
	}
}

}
