#include "../precompiled.hpp"
#include "dungeon_object.hpp"
#include <poseidon/multi_index_map.hpp>
#include <string.h>
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>

namespace EmperyDungeon {

namespace {
	MULTI_INDEX_MAP(DungeonObjectTypeMap, Data::DungeonObjectType,
		UNIQUE_MEMBER_INDEX(dungeon_object_type_id)
	)
	boost::weak_ptr<const DungeonObjectTypeMap> g_dungeon_object_map;
	const char DUNGEON_OBJECT_TYPE_FILE[] = "Arm";

	MULTI_INDEX_MAP(DungeonObjectRelativeMap, Data::DungeonObjectRelative,
		UNIQUE_MEMBER_INDEX(attack_type)
	)
	boost::weak_ptr<const DungeonObjectRelativeMap> g_dungeon_object_relative_map;
	const char DUNGEON_OBJECT_RELATIVE_FILE[] = "Arm_relative";

	const char DUNGEON_OBJECT_TYPE_MONSTER_FILE[] = "dungeon_monster";
	MULTI_INDEX_MAP(DungeonObjectTypeMonsterMap, Data::DungeonObjectTypeMonster,
		UNIQUE_MEMBER_INDEX(dungeon_object_type_id)
	)
	boost::weak_ptr<const DungeonObjectTypeMonsterMap> g_dungeon_object_type_monster_map;

	const char DUNGEON_OBJECT_AI_DATA_FILE[] = "AI";
	MULTI_INDEX_MAP(DungeonObjectAiDataMap, Data::DungeonObjectAi,
		UNIQUE_MEMBER_INDEX(unique_id)
	)
	boost::weak_ptr<const DungeonObjectAiDataMap> g_dungeon_object_ai_data_map;

	MODULE_RAII_PRIORITY(handles, 1000){
		auto csv = Data::sync_load_data(DUNGEON_OBJECT_TYPE_FILE);
		const auto dungeon_object_map         =  boost::make_shared<DungeonObjectTypeMap>();
		const auto dungeon_object_monster_map =  boost::make_shared<DungeonObjectTypeMonsterMap>();
		while(csv.fetch_row()){
			Data::DungeonObjectType elem = { };
			csv.get(elem.dungeon_object_type_id,            "arm_id");
			csv.get(elem.category_id,                       "arm_type");
			csv.get(elem.attack,                            "attack");
			csv.get(elem.defence,                           "defence");
			csv.get(elem.speed,                             "speed");
			csv.get(elem.shoot_range,                       "shoot_range");
			csv.get(elem.attack_speed,                      "attack_speed");
			csv.get(elem.attack_plus,                       "attack_plus");
			csv.get(elem.harvest_speed,                     "collect_speed");
			csv.get(elem.doge_rate,                         "arm_dodge");
			csv.get(elem.critical_rate,                     "arm_crit");
			csv.get(elem.critical_damage_plus_rate,         "arm_crit_damege");
			csv.get(elem.attack_type,                       "arm_attack_type");
			csv.get(elem.defence_type,                      "arm_def_type");
			csv.get(elem.max_soldier_count,                 "force_mnax");
			csv.get(elem.hp,                                "hp");
			csv.get(elem.ai_id,                             "ai_id");
			csv.get(elem.view,                              "view");

			if(!dungeon_object_map->insert(std::move(elem)).second){
				LOG_EMPERY_DUNGEON_ERROR("Duplicate DungeonObjectType: dungeon_object_type_id = ", elem.dungeon_object_type_id);
				DEBUG_THROW(Exception, sslit("Duplicate DungeonObjectType"));
			}
		}

		auto csvMonster = Data::sync_load_data(DUNGEON_OBJECT_TYPE_MONSTER_FILE);
		while(csvMonster.fetch_row()){
			Data::DungeonObjectType elem = { };
			csvMonster.get(elem.dungeon_object_type_id,            "arm_id");
			csvMonster.get(elem.category_id,                       "arm_type");
			csvMonster.get(elem.attack,                            "attack");
			csvMonster.get(elem.defence,                           "defence");
			csvMonster.get(elem.speed,                             "speed");
			csvMonster.get(elem.shoot_range,                       "shoot_range");
			csvMonster.get(elem.attack_speed,                      "attack_speed");
			csvMonster.get(elem.attack_plus,                       "attack_plus");
			csvMonster.get(elem.doge_rate,                         "arm_dodge");
			csvMonster.get(elem.critical_rate,                     "arm_crit");
			csvMonster.get(elem.critical_damage_plus_rate,         "arm_crit_damege");
			csvMonster.get(elem.attack_type,                       "arm_attack_type");
			csvMonster.get(elem.defence_type,                      "arm_def_type");
			csvMonster.get(elem.max_soldier_count,                  "force_mnax");
			csvMonster.get(elem.hp,                                "hp");
			csvMonster.get(elem.ai_id,                             "ai_id");
			csvMonster.get(elem.view,                              "view");
			
			Poseidon::JsonArray array;
			csvMonster.get(array,                                         "trigger_condition");
			for(unsigned i = 0; i < array.size(); ++i){
				auto &range_array = array.at(i).get<Poseidon::JsonArray>();
				auto high = static_cast<std::uint32_t>(range_array.at(0).get<double>());
				auto low  = static_cast<std::uint32_t>(range_array.at(1).get<double>());
				elem.skill_trigger.push_back(std::make_pair(high,low));
			}
			
			array.clear();
			csvMonster.get(array,                                         "skill");
			if(array.size() != elem.skill_trigger.size()){
				LOG_EMPERY_DUNGEON_ERROR("skill trigger size != skill size, dungeon_object_type_id = ", elem.dungeon_object_type_id);
				DEBUG_THROW(Exception, sslit("skill trigger size != skill size"));
			}
			for(unsigned i = 0; i < array.size(); ++i){
				auto skills_array = array.at(i).get<Poseidon::JsonArray>();
				std::vector<DungeonMonsterSkillId> skills;
				for(unsigned j = 0; j < skills_array.size(); ++j){
					auto skill_id = static_cast<std::uint64_t>(skills_array.at(j).get<double>());
					skills.push_back(DungeonMonsterSkillId(skill_id));
				}
				elem.skills.push_back(std::move(skills));
			}

			if(!dungeon_object_map->insert(std::move(elem)).second){
				LOG_EMPERY_DUNGEON_ERROR("Duplicate DungeonObjectType: dungeon_object_type_id = ", elem.dungeon_object_type_id);
				DEBUG_THROW(Exception, sslit("Duplicate DungoenObjectType"));
			}
			Data::DungeonObjectTypeMonster monster_elem = {};
			csvMonster.get(monster_elem.dungeon_object_type_id,                "arm_id");
			csvMonster.get(monster_elem.arm_relative_id,                   "arm_relative");
			if(!dungeon_object_monster_map->insert(std::move(monster_elem)).second){
				LOG_EMPERY_DUNGEON_ERROR("Duplicate DungeonObjectTypeMonster: dungeon_object_type_id = ", monster_elem.dungeon_object_type_id);
				DEBUG_THROW(Exception, sslit("Duplicate DungeonObjectTypeMonster"));
			}
		}

		g_dungeon_object_map = dungeon_object_map;
		g_dungeon_object_type_monster_map = dungeon_object_monster_map;
		handles.push(dungeon_object_map);
		handles.push(dungeon_object_monster_map);


		auto csvRelative = Data::sync_load_data(DUNGEON_OBJECT_RELATIVE_FILE);
		const auto dungeon_object_relative_map = boost::make_shared<DungeonObjectRelativeMap>();
		while(csvRelative.fetch_row()){
			Data::DungeonObjectRelative elem = { };
			csvRelative.get(elem.attack_type,         "arm_attack_type");

			Poseidon::JsonObject object;
			csvRelative.get(object, "arm_def_type");
			elem.arm_relative.reserve(object.size());
			for(auto it = object.begin(); it != object.end(); ++it){
				const auto defence_type = boost::lexical_cast<unsigned>(it->first);
				const auto relateive = it->second.get<double>();
				if(!elem.arm_relative.emplace(defence_type, relateive).second){
					LOG_EMPERY_DUNGEON_ERROR("Duplicate arm  relateive: attack_type = ", elem.attack_type , "defence type =",defence_type);
					DEBUG_THROW(Exception, sslit("Duplicate attack type"));
				}
			}

			if(!dungeon_object_relative_map->insert(std::move(elem)).second){
				LOG_EMPERY_DUNGEON_ERROR("Duplicate arm relative:attack type = ", elem.attack_type);
				DEBUG_THROW(Exception, sslit("Duplicate DungeonRelative"));
			}
		}
		g_dungeon_object_relative_map = dungeon_object_relative_map;
		handles.push(dungeon_object_relative_map);

		const auto dungeon_object_ai_map = boost::make_shared<DungeonObjectAiDataMap>();
		auto csvAi = Data::sync_load_data(DUNGEON_OBJECT_AI_DATA_FILE);
		while(csvAi.fetch_row()){
			Data::DungeonObjectAi elem = { };
			csvAi.get(elem.unique_id,         "ai_id");
			csvAi.get(elem.ai_type,           "ai_type");
			csvAi.get(elem.params,            "ai_numerical");
			csvAi.get(elem.params2,           "ai_numerical_2");
			csvAi.get(elem.ai_linkage,        "ai_linkage");
			csvAi.get(elem.ai_Intelligence,   "ai_Intelligence");
			if(!dungeon_object_ai_map->insert(std::move(elem)).second){
				LOG_EMPERY_DUNGEON_ERROR("Duplicate ai data: ai_id = ", elem.unique_id);
				DEBUG_THROW(Exception, sslit("Duplicate ai data"));
			}
		}
		g_dungeon_object_ai_data_map = dungeon_object_ai_map;
		handles.push(dungeon_object_ai_map);
	}
}

namespace Data {
	boost::shared_ptr<const DungeonObjectType> DungeonObjectType::get(DungeonObjectTypeId dungeon_object_type_id){
		PROFILE_ME;

		const auto dungeon_object_map = g_dungeon_object_map.lock();
		if(!dungeon_object_map){
			LOG_EMPERY_DUNGEON_WARNING("DungeonObjectTypeMap has not been loaded.");
			return { };
		}

		const auto it = dungeon_object_map->find<0>(dungeon_object_type_id);
		if(it == dungeon_object_map->end<0>()){
			LOG_EMPERY_DUNGEON_DEBUG("DungeonObjectType not found: dungeon_object_type_id = ", dungeon_object_type_id);
			return { };
		}
		return boost::shared_ptr<const DungeonObjectType>(dungeon_object_map, &*it);
	}

	boost::shared_ptr<const DungeonObjectType> DungeonObjectType::require(DungeonObjectTypeId dungeon_object_type_id){
		PROFILE_ME;

		auto ret = get(dungeon_object_type_id);
		if(!ret){
			DEBUG_THROW(Exception, sslit("DungeonObjectType not found"));
		}
		return ret;
	}

	double DungeonObjectRelative::get_relative(unsigned attack_type,unsigned defence_type){
		PROFILE_ME;
		const auto dungeon_object_relative_map = g_dungeon_object_relative_map.lock();
		if(!dungeon_object_relative_map){
			return 1.0;
		}

		const auto it = dungeon_object_relative_map->find<0>(attack_type);
		if(it == dungeon_object_relative_map->end<0>()){
			return 1.0;
		}
		const auto relatives_map = (*it).arm_relative;
		const auto relative_it = relatives_map.find(defence_type);
		if(relative_it == relatives_map.end()){
			return 1.0;
		}
		return (*relative_it).second;
	}

	boost::shared_ptr<const DungeonObjectTypeMonster> DungeonObjectTypeMonster::get(DungeonObjectTypeId dungeon_object_type_id){
		PROFILE_ME;

		const auto dungeon_object_monster_map = g_dungeon_object_type_monster_map.lock();
		if(!dungeon_object_monster_map){
			LOG_EMPERY_DUNGEON_WARNING("DungeonObjectTypeMonsterMap has not been loaded.");
			return { };
		}

		const auto it = dungeon_object_monster_map->find<0>(dungeon_object_type_id);
		if(it == dungeon_object_monster_map->end<0>()){
			return { };
		}
		return boost::shared_ptr<const DungeonObjectTypeMonster>(dungeon_object_monster_map, &*it);
	}

	boost::shared_ptr<const DungeonObjectAi> DungeonObjectAi::get(std::uint64_t unique_id){
		PROFILE_ME;

		const auto dungeon_object_ai_map = g_dungeon_object_ai_data_map.lock();
		if(!dungeon_object_ai_map){
			LOG_EMPERY_DUNGEON_WARNING("dungeon_object_ai_map has not been loaded.");
			return { };
		}
		const auto it = dungeon_object_ai_map->find<0>(unique_id);
		if(it == dungeon_object_ai_map->end<0>()){
			return { };
		}
		return boost::shared_ptr<const DungeonObjectAi>(dungeon_object_ai_map, &*it);
	}
	boost::shared_ptr<const DungeonObjectAi> DungeonObjectAi::require(std::uint64_t unique_id){
		PROFILE_ME;

		auto ret = get(unique_id);
		if(!ret){
			LOG_EMPERY_DUNGEON_WARNING("DungeonObject ai not found, ai_id = ", unique_id);
			DEBUG_THROW(Exception, sslit("DungeonObject ai not found"));
		}
		return ret;
	}
}

}
