#include "../precompiled.hpp"
#include "map_object.hpp"
#include <poseidon/multi_index_map.hpp>
#include <string.h>
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>
#include "../data_session.hpp"

namespace EmperyCenter {

namespace {
	MULTI_INDEX_MAP(MapObjectMap, Data::MapObject,
		UNIQUE_MEMBER_INDEX(map_object_type_id)
	)
	boost::weak_ptr<const MapObjectMap> g_map_object_map;
	const char MAP_OBJECT_TYPE_FILE[] = "Arm";

	MODULE_RAII_PRIORITY(handles, 1000){
		auto csv = Data::sync_load_data(MAP_OBJECT_TYPE_FILE);
		const auto map_object_map = boost::make_shared<MapObjectMap>();
		while(csv.fetch_row()){
			Data::MapObject elem = { };

			csv.get(elem.map_object_type_id, "arm_id");

			csv.get(elem.speed,              "speed");
			csv.get(elem.harvest_speed,      "collect_speed");

			if(!map_object_map->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate MapObject: map_object_type_id = ", elem.map_object_type_id);
				DEBUG_THROW(Exception, sslit("Duplicate MapObject"));
			}
		}
		g_map_object_map = map_object_map;
		handles.push(map_object_map);
		auto servlet = DataSession::create_servlet(MAP_OBJECT_TYPE_FILE, Data::encode_csv_as_json(csv, "arm_id"));
		handles.push(std::move(servlet));
	}
}

namespace Data {
	boost::shared_ptr<const MapObject> MapObject::get(MapObjectTypeId map_object_type_id){
		PROFILE_ME;

		const auto map_object_map = g_map_object_map.lock();
		if(!map_object_map){
			LOG_EMPERY_CENTER_WARNING("MapObjectMap has not been loaded.");
			return { };
		}

		const auto it = map_object_map->find<0>(map_object_type_id);
		if(it == map_object_map->end<0>()){
			LOG_EMPERY_CENTER_DEBUG("MapObject not found: map_object_type_id = ", map_object_type_id);
			return { };
		}
		return boost::shared_ptr<const MapObject>(map_object_map, &*it);
	}
	boost::shared_ptr<const MapObject> MapObject::require(MapObjectTypeId map_object_type_id){
		PROFILE_ME;

		auto ret = get(map_object_type_id);
		if(!ret){
			DEBUG_THROW(Exception, sslit("MapObject not found"));
		}
		return ret;
	}
}

}
