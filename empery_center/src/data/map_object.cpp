#include "../precompiled.hpp"
#include "map_object.hpp"
#include <poseidon/multi_index_map.hpp>
#include <string.h>
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>
#include "../data_session.hpp"

namespace EmperyCenter {

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

			csv.get(elem.speed,              "speed");
			csv.get(elem.harvest_speed,      "collect_speed");

			Poseidon::JsonObject object;
			csv.get(object, "arm_need");
			elem.prerequisite.reserve(object.size());
			for(auto it = object.begin(); it != object.end(); ++it){
				const auto building_id = boost::lexical_cast<BuildingId>(it->first);
				const auto building_level = static_cast<unsigned>(it->second.get<double>());
				if(!elem.prerequisite.emplace(building_id, building_level).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate prerequisite: building_id = ", building_id);
					DEBUG_THROW(Exception, sslit("Duplicate prerequisite"));
				}
			}
			object.clear();
			csv.get(object, "levelup_resource");
			elem.enability_cost.reserve(object.size());
			for(auto it = object.begin(); it != object.end(); ++it){
				const auto resource_id = boost::lexical_cast<ResourceId>(it->first);
				const auto resource_amount = static_cast<std::uint64_t>(it->second.get<double>());
				if(!elem.enability_cost.emplace(resource_id, resource_amount).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate enability resource cost: resource_id = ", resource_id);
					DEBUG_THROW(Exception, sslit("Duplicate enability resource cost"));
				}
			}

			object.clear();
			csv.get(object, "recruit_resource");
			elem.production_cost.reserve(object.size());
			for(auto it = object.begin(); it != object.end(); ++it){
				const auto resource_id = boost::lexical_cast<ResourceId>(it->first);
				const auto resource_amount = static_cast<std::uint64_t>(it->second.get<double>());
				if(!elem.production_cost.emplace(resource_id, resource_amount).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate production resource cost: resource_id = ", resource_id);
					DEBUG_THROW(Exception, sslit("Duplicate production resource cost"));
				}
			}
			object.clear();
			csv.get(object, "maintenance_resource");
			elem.maintenance_cost.reserve(object.size());
			for(auto it = object.begin(); it != object.end(); ++it){
				const auto resource_id = boost::lexical_cast<ResourceId>(it->first);
				const auto resource_amount = static_cast<std::uint64_t>(it->second.get<double>());
				if(!elem.maintenance_cost.emplace(resource_id, resource_amount).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate maintenance resource cost: resource_id = ", resource_id);
					DEBUG_THROW(Exception, sslit("Duplicate maintenance resource cost"));
				}
			}

			csv.get(elem.previous_id,        "soldiers_need");
			csv.get(elem.production_time,    "levy_time");
			csv.get(elem.factory_id,         "city_camp");
			csv.get(elem.max_rally_count,    "force_mnax");

			if(!map_object_map->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate MapObjectType: map_object_type_id = ", elem.map_object_type_id);
				DEBUG_THROW(Exception, sslit("Duplicate MapObjectType"));
			}
		}
		g_map_object_map = map_object_map;
		handles.push(map_object_map);
		auto servlet = DataSession::create_servlet(MAP_OBJECT_TYPE_FILE, Data::encode_csv_as_json(csv, "arm_id"));
		handles.push(std::move(servlet));
	}
}

namespace Data {
	boost::shared_ptr<const MapObjectType> MapObjectType::get(MapObjectTypeId map_object_type_id){
		PROFILE_ME;

		const auto map_object_map = g_map_object_map.lock();
		if(!map_object_map){
			LOG_EMPERY_CENTER_WARNING("MapObjectTypeMap has not been loaded.");
			return { };
		}

		const auto it = map_object_map->find<0>(map_object_type_id);
		if(it == map_object_map->end<0>()){
			LOG_EMPERY_CENTER_DEBUG("MapObjectType not found: map_object_type_id = ", map_object_type_id);
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
