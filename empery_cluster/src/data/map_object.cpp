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

	MODULE_RAII_PRIORITY(handles, 1000){
		auto csv = Data::sync_load_data(MAP_OBJECT_TYPE_FILE);
		const auto map_object_map = boost::make_shared<MapObjectTypeMap>();
		while(csv.fetch_row()){
			Data::MapObjectType elem = { };
			csv.get(elem.map_object_type_id, "arm_id");

			csv.get(elem.attack,              "attack");
			csv.get(elem.defence,             "defence");
			csv.get(elem.shoot_range,         "shoot_range");
			csv.get(elem.attack_speed,        "attack_speed");
			csv.get(elem.first_attack,        "first_attack");
			

			if(!map_object_map->insert(std::move(elem)).second){
				LOG_EMPERY_CLUSTER_ERROR("Duplicate MapObjectType: map_object_type_id = ", elem.map_object_type_id);
				DEBUG_THROW(Exception, sslit("Duplicate MapObjectType"));
			}
		}
		g_map_object_map = map_object_map;
		handles.push(map_object_map);
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
}

}
