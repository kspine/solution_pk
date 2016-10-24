#include "../precompiled.hpp"
#include "dungeon_buff.hpp"
#include <poseidon/multi_index_map.hpp>
#include <string.h>
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>

namespace EmperyDungeon {

namespace {
	MULTI_INDEX_MAP(DungeonBuffContainer, Data::DungeonBuff,
		UNIQUE_MEMBER_INDEX(buff_id)
	)
	const char DUNGEON_BUFF_FILE[] = "dungeons_buff";
	boost::weak_ptr<const DungeonBuffContainer> g_dungeon_buff_container;

	MODULE_RAII_PRIORITY(handles, 1000){
		const auto dungeon_buff_container = boost::make_shared<DungeonBuffContainer>();
		auto csv = Data::sync_load_data(DUNGEON_BUFF_FILE);
		while(csv.fetch_row()){
			Data::DungeonBuff elem = { };
			csv.get(elem.buff_id,           "buff_id");
			csv.get(elem.target,            "object");
			csv.get(elem.continue_time,     "time");
			csv.get(elem.attribute_id,      "Attribute_type");
			csv.get(elem.value,             "value");
			auto buff_id = elem.buff_id;
			if(!dungeon_buff_container->insert(std::move(elem)).second){
				LOG_EMPERY_DUNGEON_ERROR("Duplicate dungeon buff = ",buff_id);
				DEBUG_THROW(Exception, sslit("Duplicate dungeon buff"));
			}
		}
		g_dungeon_buff_container = dungeon_buff_container;
		handles.push(dungeon_buff_container);
	}
}

namespace Data {
	boost::shared_ptr<const DungeonBuff> DungeonBuff::get(DungeonBuffTypeId buff_id){
		PROFILE_ME;

		const auto dungeon_buff_container = g_dungeon_buff_container.lock();
		if(!dungeon_buff_container){
			LOG_EMPERY_DUNGEON_WARNING("dungeon_buff_container has not been loaded.");
			return { };
		}

		const auto it = dungeon_buff_container->find<0>(buff_id);
		if(it == dungeon_buff_container->end<0>()){
			LOG_EMPERY_DUNGEON_TRACE("dugeon buff not found: buff_id = ", buff_id);
			return { };
		}
		return boost::shared_ptr<const DungeonBuff>(dungeon_buff_container, &*it);
	}

	boost::shared_ptr<const DungeonBuff> DungeonBuff::require(DungeonBuffTypeId buff_id){
		PROFILE_ME;

		auto ret = get(buff_id);
		if(!ret){
			LOG_EMPERY_DUNGEON_WARNING("dungeon buff not found: buff_id = ", buff_id);
			DEBUG_THROW(Exception, sslit("dungeon buff not found"));
		}
		return ret;
	}
}
}
