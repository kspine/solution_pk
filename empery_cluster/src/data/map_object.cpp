#include "../precompiled.hpp"
#include "map_object.hpp"
#include <poseidon/multi_index_map.hpp>
#include <string.h>
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>

namespace EmperyCluster {

namespace {
	MULTI_INDEX_MAP(MapObjectTypeMap, Data::MapObjectType,
		UNIQUE_MEMBER_INDEX(map_object_type_id)
	)
	boost::weak_ptr<const MapObjectTypeMap> g_map_object_map;
	const char MAP_OBJECT_TYPE_FILE[] = "Arm";

	MULTI_INDEX_MAP(MapObjectRelativeMap, Data::MapObjectRelative,
		UNIQUE_MEMBER_INDEX(attack_type)
	)
	boost::weak_ptr<const MapObjectRelativeMap> g_map_object_relative_map;
	const char MAP_OBJECT_RELATIVE_FILE[] = "Arm_relative";

	const char MAP_OBJECT_TYPE_MONSTER_FILE[] = "monster";
	MULTI_INDEX_MAP(MapObjectTypeMonsterMap, Data::MapObjectTypeMonster,
		UNIQUE_MEMBER_INDEX(map_object_type_id)
	)
	boost::weak_ptr<const MapObjectTypeMonsterMap> g_map_object_type_monster_map;

	const char MAP_OBJECT_TYPE_BUILDING_FILE[] = "Building_combat_attributes";
	const char MAP_OBJECT_TYPE_BUILDING_TOWER_FILE[] = "Building_towers";
	const char MAP_OBJECT_TYPE_BUILDING_CASTLE_FILE[] = "Building_castel";
	const char MAP_OBJECT_TYPE_BUILDING_BUNKER_FILE[] = "Building_bunker";
	const char MAP_OBJECT_TYPE_BUILDING_WAREHOUSE_FILE[] = "corps_house";
	MULTI_INDEX_MAP(MapObjectTypeBuildingMap, Data::MapObjectTypeBuilding,
		UNIQUE_MEMBER_INDEX(type_level)
		MULTI_MEMBER_INDEX(map_object_type_id)
	)
	boost::weak_ptr<const MapObjectTypeBuildingMap> g_map_object_type_building_map;

	const char MAP_OBJECT_AI_DATA_FILE[] = "AI";
	MULTI_INDEX_MAP(MapObjectAiDataMap, Data::MapObjectAi,
		UNIQUE_MEMBER_INDEX(unique_id)
	)
	boost::weak_ptr<const MapObjectAiDataMap> g_map_object_ai_data_map;

	MODULE_RAII_PRIORITY(handles, 1000){
		auto csv = Data::sync_load_data(MAP_OBJECT_TYPE_FILE);
		const auto map_object_map         =  boost::make_shared<MapObjectTypeMap>();
		const auto map_object_monster_map =  boost::make_shared<MapObjectTypeMonsterMap>();
		while(csv.fetch_row()){
			Data::MapObjectType elem = { };
			csv.get(elem.map_object_type_id,                "arm_id");
			csv.get(elem.category_id,                       "arm_type");
			csv.get(elem.map_object_chassis_id,             "arm_class");
			csv.get(elem.attack,                            "attack");
			csv.get(elem.defence,                           "defence");
			csv.get(elem.speed,                             "speed");
			csv.get(elem.shoot_range,                       "shoot_range");
			csv.get(elem.attack_speed,                      "attack_speed");
			csv.get(elem.attack_plus,                      "attack_plus");
			csv.get(elem.harvest_speed,                    "collect_speed");
			csv.get(elem.doge_rate,                         "arm_dodge");
			csv.get(elem.critical_rate,                     "arm_crit");
			csv.get(elem.critical_damage_plus_rate,         "arm_crit_damege");
			csv.get(elem.attack_type,         				"arm_attack_type");
			csv.get(elem.defence_type,                      "arm_def_type");
			csv.get(elem.hp,                                "hp");
			csv.get(elem.ai_id,                             "ai_id");
			csv.get(elem.view,                              "view");

			if(!map_object_map->insert(std::move(elem)).second){
				LOG_EMPERY_CLUSTER_ERROR("Duplicate MapObjectType: map_object_type_id = ", elem.map_object_type_id);
				DEBUG_THROW(Exception, sslit("Duplicate MapObjectType"));
			}
		}

		auto csvMonster = Data::sync_load_data(MAP_OBJECT_TYPE_MONSTER_FILE);
		while(csvMonster.fetch_row()){
			Data::MapObjectType elem = { };
			csvMonster.get(elem.map_object_type_id,                "arm_id");
			csvMonster.get(elem.category_id,                       "arm_type");
			csvMonster.get(elem.map_object_chassis_id,             "arm_class");
			csvMonster.get(elem.attack,                            "attack");
			csvMonster.get(elem.defence,                           "defence");
			csvMonster.get(elem.speed,                             "speed");
			csvMonster.get(elem.shoot_range,                       "shoot_range");
			csvMonster.get(elem.attack_speed,                      "attack_speed");
			csvMonster.get(elem.attack_plus,                       "attack_plus");
			csvMonster.get(elem.doge_rate,                         "arm_dodge");
			csvMonster.get(elem.critical_rate,                     "arm_crit");
			csvMonster.get(elem.critical_damage_plus_rate,         "arm_crit_damege");
			csvMonster.get(elem.attack_type,         			   "arm_attack_type");
			csvMonster.get(elem.defence_type,                      "arm_def_type");
			csvMonster.get(elem.hp,                      		   "hp");
			csvMonster.get(elem.ai_id,                             "ai_id");
			csvMonster.get(elem.view,                              "view");

			if(!map_object_map->insert(std::move(elem)).second){
				LOG_EMPERY_CLUSTER_ERROR("Duplicate MapObjectType: map_object_type_id = ", elem.map_object_type_id);
				DEBUG_THROW(Exception, sslit("Duplicate MapObjectType"));
			}
			Data::MapObjectTypeMonster monster_elem = {};
			csvMonster.get(monster_elem.map_object_type_id,                "arm_id");
			csvMonster.get(monster_elem.arm_relative_id,                   "arm_relative");
			csvMonster.get(monster_elem.level,                             "arm_level");
			if(!map_object_monster_map->insert(std::move(monster_elem)).second){
				LOG_EMPERY_CLUSTER_ERROR("Duplicate MapObjectTypeMonster: map_object_type_id = ", monster_elem.map_object_type_id);
				DEBUG_THROW(Exception, sslit("Duplicate MapObjectTypeMonster"));
			}
		}

		auto csvBuilding = Data::sync_load_data(MAP_OBJECT_TYPE_BUILDING_FILE);
		while(csvBuilding.fetch_row()){
			Data::MapObjectType elem = { };
			csvBuilding.get(elem.map_object_type_id,                "id");
			csvBuilding.get(elem.category_id,                       "arm_type");
			csvBuilding.get(elem.attack,                            "attack");
			csvBuilding.get(elem.defence,                           "defence");
			//csvBuilding.get(elem.speed,                           "speed");
			csvBuilding.get(elem.shoot_range,                       "shoot_range");
			csvBuilding.get(elem.attack_speed,                      "attack_speed");
			csvBuilding.get(elem.attack_plus,                       "attack_plus");
			csvBuilding.get(elem.doge_rate,                         "arm_dodge");
			csvBuilding.get(elem.critical_rate,                     "arm_crit");
			csvBuilding.get(elem.critical_damage_plus_rate,         "arm_crit_damege");
			csvBuilding.get(elem.attack_type,         			    "arm_attack_type");
			csvBuilding.get(elem.defence_type,                      "arm_def_type");
			csvBuilding.get(elem.hp,                      		    "hp");
			csvBuilding.get(elem.ai_id,                             "ai_id");
			csvBuilding.get(elem.view,                              "view");

			if(!map_object_map->insert(std::move(elem)).second){
				LOG_EMPERY_CLUSTER_ERROR("Duplicate MapObjectType: map_object_type_id = ", elem.map_object_type_id);
				DEBUG_THROW(Exception, sslit("Duplicate MapObjectType"));
			}
		}

		g_map_object_map = map_object_map;
		g_map_object_type_monster_map = map_object_monster_map;
		handles.push(map_object_map);
		handles.push(map_object_monster_map);


		auto csvRelative = Data::sync_load_data(MAP_OBJECT_RELATIVE_FILE);
		const auto map_object_relative_map = boost::make_shared<MapObjectRelativeMap>();
		while(csvRelative.fetch_row()){
			Data::MapObjectRelative elem = { };
			csvRelative.get(elem.attack_type,         "arm_attack_type");

			Poseidon::JsonObject object;
			csvRelative.get(object, "arm_def_type");
			elem.arm_relative.reserve(object.size());
			for(auto it = object.begin(); it != object.end(); ++it){
				const auto defence_type = boost::lexical_cast<unsigned>(it->first);
				const auto relateive = it->second.get<double>();
				if(!elem.arm_relative.emplace(defence_type, relateive).second){
					LOG_EMPERY_CLUSTER_ERROR("Duplicate arm  relateive: attack_type = ", elem.attack_type , "defence type =",defence_type);
					DEBUG_THROW(Exception, sslit("Duplicate attack type"));
				}
			}

			if(!map_object_relative_map->insert(std::move(elem)).second){
				LOG_EMPERY_CLUSTER_ERROR("Duplicate arm relative:attack type = ", elem.attack_type);
				DEBUG_THROW(Exception, sslit("Duplicate MapRelative"));
			}
		}
		g_map_object_relative_map = map_object_relative_map;
		handles.push(map_object_relative_map);

		auto csvTower = Data::sync_load_data(MAP_OBJECT_TYPE_BUILDING_TOWER_FILE);
		const auto map_object_building_map = boost::make_shared<MapObjectTypeBuildingMap>();
		while(csvTower.fetch_row()){
			Data::MapObjectTypeBuilding elem = { };
			csvTower.get(elem.map_object_type_id,         "building_id");
			csvTower.get(elem.level,                      "building_level");
			csvTower.get(elem.arm_type_id,                "building_combat_attributes");
			elem.type_level = std::make_pair(elem.map_object_type_id, elem.level);

			if(!map_object_building_map->insert(std::move(elem)).second){
				LOG_EMPERY_CLUSTER_ERROR("Duplicate tower: map_object_type_id = ", elem.map_object_type_id, " level:",elem.level);
				DEBUG_THROW(Exception, sslit("Duplicate tower"));
			}
		}
		auto csvCastle = Data::sync_load_data(MAP_OBJECT_TYPE_BUILDING_CASTLE_FILE);
		while(csvCastle.fetch_row()){
			Data::MapObjectTypeBuilding elem = { };
			csvCastle.get(elem.map_object_type_id,         "building_id");
			csvCastle.get(elem.level,                      "building_level");
			csvCastle.get(elem.arm_type_id,                "building_combat_attributes");
			elem.type_level = std::make_pair(elem.map_object_type_id, elem.level);

			if(!map_object_building_map->insert(std::move(elem)).second){
				LOG_EMPERY_CLUSTER_ERROR("Duplicate castle: map_object_type_id = ", elem.map_object_type_id, " level:",elem.level);
				DEBUG_THROW(Exception, sslit("Duplicate castle"));
			}
		}

		auto csvBunker = Data::sync_load_data(MAP_OBJECT_TYPE_BUILDING_BUNKER_FILE);
		while(csvBunker.fetch_row()){
			Data::MapObjectTypeBuilding elem = { };
			csvBunker.get(elem.map_object_type_id,         "building_id");
			csvBunker.get(elem.level,                      "building_level");
			csvBunker.get(elem.arm_type_id,                "building_combat_attributes");
			elem.type_level = std::make_pair(elem.map_object_type_id, elem.level);

			if(!map_object_building_map->insert(std::move(elem)).second){
				LOG_EMPERY_CLUSTER_ERROR("Duplicate bunker: map_object_type_id = ", elem.map_object_type_id, " level:",elem.level);
				DEBUG_THROW(Exception, sslit("Duplicate bunker"));
			}
		}

		auto csvWareHouse = Data::sync_load_data(MAP_OBJECT_TYPE_BUILDING_WAREHOUSE_FILE);
		while(csvWareHouse.fetch_row()){
			Data::MapObjectTypeBuilding elem = { };
			csvWareHouse.get(elem.map_object_type_id,         "building_id");
			csvWareHouse.get(elem.level,                      "house_level");
			csvWareHouse.get(elem.arm_type_id,                "building_combat_attributes");
			elem.type_level = std::make_pair(elem.map_object_type_id, elem.level);

			if(!map_object_building_map->insert(std::move(elem)).second){
				LOG_EMPERY_CLUSTER_ERROR("Duplicate WareHouse: map_object_type_id = ", elem.map_object_type_id, " level:",elem.level);
				DEBUG_THROW(Exception, sslit("Duplicate WareHouse"));
			}
		}


		g_map_object_type_building_map = map_object_building_map;
		handles.push(map_object_building_map);

		const auto map_object_ai_map = boost::make_shared<MapObjectAiDataMap>();
		auto csvAi = Data::sync_load_data(MAP_OBJECT_AI_DATA_FILE);
		while(csvAi.fetch_row()){
			Data::MapObjectAi elem = { };
			csvAi.get(elem.unique_id,         "ai_id");
			csvAi.get(elem.ai_type,           "ai_type");
			csvAi.get(elem.params,            "ai_numerical");

			if(!map_object_ai_map->insert(std::move(elem)).second){
				LOG_EMPERY_CLUSTER_ERROR("Duplicate ai data: ai_id = ", elem.unique_id);
				DEBUG_THROW(Exception, sslit("Duplicate ai data"));
			}
		}
		g_map_object_ai_data_map = map_object_ai_map;
		handles.push(map_object_ai_map);
	}
}

namespace Data {
	boost::shared_ptr<const MapObjectType> MapObjectType::get(MapObjectTypeId map_object_type_id){
		PROFILE_ME;

		const auto map_object_map = g_map_object_map.lock();
		if(!map_object_map){
			LOG_EMPERY_CLUSTER_WARNING("MapObjectTypeMap has not been loaded.");
			return { };
		}

		const auto it = map_object_map->find<0>(map_object_type_id);
		if(it == map_object_map->end<0>()){
			LOG_EMPERY_CLUSTER_DEBUG("MapObjectType not found: map_object_type_id = ", map_object_type_id);
			return { };
		}
		return boost::shared_ptr<const MapObjectType>(map_object_map, &*it);
	}

	boost::shared_ptr<const MapObjectType> MapObjectType::require(MapObjectTypeId map_object_type_id){
		PROFILE_ME;

		auto ret = get(map_object_type_id);
		if(!ret){
			DEBUG_THROW(Exception, sslit("MapObjectType not found"));
		}
		return ret;
	}

	double MapObjectRelative::get_relative(unsigned attack_type,unsigned defence_type){
		PROFILE_ME;
		const auto map_object_relative_map = g_map_object_relative_map.lock();
		if(!map_object_relative_map){
			return 1.0;
		}

		const auto it = map_object_relative_map->find<0>(attack_type);
		if(it == map_object_relative_map->end<0>()){
			return 1.0;
		}
		const auto relatives_map = (*it).arm_relative;
		const auto relative_it = relatives_map.find(defence_type);
		if(relative_it == relatives_map.end()){
			return 1.0;
		}
		return (*relative_it).second;
	}

	boost::shared_ptr<const MapObjectTypeMonster> MapObjectTypeMonster::get(MapObjectTypeId map_object_type_id){
		PROFILE_ME;

		const auto map_object_monster_map = g_map_object_type_monster_map.lock();
		if(!map_object_monster_map){
			LOG_EMPERY_CLUSTER_WARNING("MapObjectTypeMonsterMap has not been loaded.");
			return { };
		}

		const auto it = map_object_monster_map->find<0>(map_object_type_id);
		if(it == map_object_monster_map->end<0>()){
			return { };
		}
		return boost::shared_ptr<const MapObjectTypeMonster>(map_object_monster_map, &*it);
	}

	boost::shared_ptr<const MapObjectTypeBuilding> MapObjectTypeBuilding::get(MapObjectTypeId map_object_type_id,std::uint32_t level){
		PROFILE_ME;

		const auto map_object_building_map = g_map_object_type_building_map.lock();
		if(!map_object_building_map){
			LOG_EMPERY_CLUSTER_WARNING("MapObjectTypeMonsterMap has not been loaded.");
			return { };
		}
		const auto it = map_object_building_map->find<0>(std::make_pair(map_object_type_id,level));
		if(it == map_object_building_map->end<0>()){
			return { };
		}
		return boost::shared_ptr<const MapObjectTypeBuilding>(map_object_building_map, &*it);
	}

	boost::shared_ptr<const MapObjectAi> MapObjectAi::get(std::uint64_t unique_id){
		PROFILE_ME;

		const auto map_object_ai_map = g_map_object_ai_data_map.lock();
		if(!map_object_ai_map){
			LOG_EMPERY_CLUSTER_WARNING("map_object_ai_map has not been loaded.");
			return { };
		}
		const auto it = map_object_ai_map->find<0>(unique_id);
		if(it == map_object_ai_map->end<0>()){
			return { };
		}
		return boost::shared_ptr<const MapObjectAi>(map_object_ai_map, &*it);
	}
}

}
