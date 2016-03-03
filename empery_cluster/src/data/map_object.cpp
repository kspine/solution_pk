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
		UNIQUE_MEMBER_INDEX(arm_type_id)
	)
	boost::weak_ptr<const MapObjectRelativeMap> g_map_object_relative_map;
	const char MAP_OBJECT_RELATIVE_FILE[] = "Arm_relative";
	
	MODULE_RAII_PRIORITY(handles, 1000){
		auto csv = Data::sync_load_data(MAP_OBJECT_TYPE_FILE);
		const auto map_object_map = boost::make_shared<MapObjectTypeMap>();
		while(csv.fetch_row()){
			Data::MapObjectType elem = { };
			csv.get(elem.map_object_type_id,                "arm_id");
			csv.get(elem.arm_type_id,                       "arm_type");
			csv.get(elem.attack,                            "attack");
			csv.get(elem.defence,                           "defence");
			csv.get(elem.shoot_range,                       "shoot_range");
			csv.get(elem.attack_speed,                      "attack_speed");
			csv.get(elem.first_attack,                      "first_attack");
			csv.get(elem.attack_plus,                      "attack_plus");
			csv.get(elem.doge_rate,                         "arm_dodge");
			csv.get(elem.critical_rate,                     "arm_crit");
			csv.get(elem.critical_damage_plus_rate,         "arm_crit_damege");

			if(!map_object_map->insert(std::move(elem)).second){
				LOG_EMPERY_CLUSTER_ERROR("Duplicate MapObjectType: map_object_type_id = ", elem.map_object_type_id);
				DEBUG_THROW(Exception, sslit("Duplicate MapObjectType"));
			}
		}
		g_map_object_map = map_object_map;
		handles.push(map_object_map);
		
		
		auto csvRelative = Data::sync_load_data(MAP_OBJECT_RELATIVE_FILE);
		const auto map_object_relative_map = boost::make_shared<MapObjectRelativeMap>();
		while(csvRelative.fetch_row()){
			Data::MapObjectRelative elem = { };
			csvRelative.get(elem.arm_type_id,         "arm_type");
			
			Poseidon::JsonObject object;
			csvRelative.get(object, "anti_relative");
			elem.arm_relative.reserve(object.size());
			for(auto it = object.begin(); it != object.end(); ++it){
				const auto relative_arm_type_id = boost::lexical_cast<ArmTypeId>(it->first);
				const auto relateive = it->second.get<double>();
				if(!elem.arm_relative.emplace(relative_arm_type_id, relateive).second){
					LOG_EMPERY_CLUSTER_ERROR("Duplicate arm  relateive: arm_type_id = ", elem.arm_type_id , "relative_arm_type_id =",relative_arm_type_id);
					DEBUG_THROW(Exception, sslit("Duplicate arm_type_id"));
				}
			}

			if(!map_object_relative_map->insert(std::move(elem)).second){
				LOG_EMPERY_CLUSTER_ERROR("Duplicate arm relative: map_object_arm_type_id = ", elem.arm_type_id);
				DEBUG_THROW(Exception, sslit("Duplicate MapObjectType"));
			}
		}
		g_map_object_relative_map = map_object_relative_map;
		handles.push(map_object_relative_map);
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
	
	double MapObjectRelative::get_relative(ArmTypeId map_object_arm_type_id,ArmTypeId relateive_arm_type_id){
		PROFILE_ME;
		const auto map_object_relative_map = g_map_object_relative_map.lock();
		if(!map_object_relative_map){
			return 1.0;
		}
		
		const auto it = map_object_relative_map->find<0>(map_object_arm_type_id);
		if(it == map_object_relative_map->end<0>()){
			return 1.0;
		}
		
		const auto relatives_map = (*it).arm_relative;
		const auto relative_it = relatives_map.find(relateive_arm_type_id);
		if(relative_it == relatives_map.end()){
			return 1.0;
		}
		return (*relative_it).second;
	}
}

}
