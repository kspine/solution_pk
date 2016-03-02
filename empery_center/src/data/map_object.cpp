#include "../precompiled.hpp"
#include "map_object.hpp"
#include <poseidon/multi_index_map.hpp>
#include <string.h>
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>
#include "../data_session.hpp"

namespace EmperyCenter {

namespace {
	using MapObjectTypeBattalionMap = boost::container::flat_map<MapObjectTypeId, Data::MapObjectTypeBattalion>;
	boost::weak_ptr<const MapObjectTypeBattalionMap> g_map_object_type_battalion_map;
	const char MAP_OBJECT_TYPE_BATTALION_FILE[] = "Arm";

	using MapObjectTypeMonsterMap = boost::container::flat_map<MapObjectTypeId, Data::MapObjectTypeMonster>;
	boost::weak_ptr<const MapObjectTypeMonsterMap> g_map_object_type_monster_map;
	const char MAP_OBJECT_TYPE_MONSTER_FILE[] = "monster";

	template<typename ElementT>
	void read_map_object_type_abstract(ElementT &elem, const Poseidon::CsvParser &csv){
		csv.get(elem.map_object_type_id,     "arm_id");
		csv.get(elem.map_object_category_id, "arm_type");

		csv.get(elem.speed,                  "speed");
	}

	MODULE_RAII_PRIORITY(handles, 1000){
		auto csv = Data::sync_load_data(MAP_OBJECT_TYPE_BATTALION_FILE);
		const auto map_object_type_battalion_map = boost::make_shared<MapObjectTypeBattalionMap>();
		while(csv.fetch_row()){
			Data::MapObjectTypeBattalion elem = { };

			read_map_object_type_abstract(elem, csv);

			csv.get(elem.harvest_speed,          "collect_speed");

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
			csv.get(elem.max_soldier_count,  "force_mnax");

			if(!map_object_type_battalion_map->emplace(elem.map_object_type_id, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate MapObjectTypeBattalion: map_object_type_id = ", elem.map_object_type_id);
				DEBUG_THROW(Exception, sslit("Duplicate MapObjectTypeBattalion"));
			}
		}
		g_map_object_type_battalion_map = map_object_type_battalion_map;
		handles.push(map_object_type_battalion_map);
		auto servlet = DataSession::create_servlet(MAP_OBJECT_TYPE_BATTALION_FILE, Data::encode_csv_as_json(csv, "arm_id"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(MAP_OBJECT_TYPE_MONSTER_FILE);
		const auto map_object_type_monster_map = boost::make_shared<MapObjectTypeMonsterMap>();
		while(csv.fetch_row()){
			Data::MapObjectTypeMonster elem = { };

			read_map_object_type_abstract(elem, csv);

			csv.get(elem.return_range,       "return_range");

			Poseidon::JsonObject object;
			csv.get(object, "drop");
			elem.reward.reserve(object.size());
			for(auto it = object.begin(); it != object.end(); ++it){
				const auto item_id = boost::lexical_cast<ItemId>(it->first);
				const auto count = static_cast<std::uint64_t>(it->second.get<double>());
				if(!elem.reward.emplace(item_id, count).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate reward item: item_id = ", item_id);
					DEBUG_THROW(Exception, sslit("Duplicate reward item"));
				}
			}

			if(!map_object_type_monster_map->emplace(elem.map_object_type_id, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate MapObjectTypeMonster: map_object_type_id = ", elem.map_object_type_id);
				DEBUG_THROW(Exception, sslit("Duplicate MapObjectTypeMonster"));
			}
		}
		g_map_object_type_monster_map = map_object_type_monster_map;
		handles.push(map_object_type_monster_map);
		servlet = DataSession::create_servlet(MAP_OBJECT_TYPE_MONSTER_FILE, Data::encode_csv_as_json(csv, "arm_id"));
		handles.push(std::move(servlet));
	}
}

namespace Data {
	boost::shared_ptr<const MapObjectTypeAbstract> MapObjectTypeAbstract::get(MapObjectTypeId map_object_type_id){
		PROFILE_ME;

		boost::shared_ptr<const MapObjectTypeAbstract> ret;
		if(!!(ret = MapObjectTypeBattalion::get(map_object_type_id))){
			return ret;
		}
		if(!!(ret = MapObjectTypeMonster::get(map_object_type_id))){
			return ret;
		}
		return ret;
	}
	boost::shared_ptr<const MapObjectTypeAbstract> MapObjectTypeAbstract::require(MapObjectTypeId map_object_type_id){
		PROFILE_ME;

		auto ret = get(map_object_type_id);
		if(!ret){
			DEBUG_THROW(Exception, sslit("MapObjectTypeAbstract not found"));
		}
		return ret;
	}

	boost::shared_ptr<const MapObjectTypeBattalion> MapObjectTypeBattalion::get(MapObjectTypeId map_object_type_id){
		PROFILE_ME;

		const auto map_object_type_battalion_map = g_map_object_type_battalion_map.lock();
		if(!map_object_type_battalion_map){
			LOG_EMPERY_CENTER_WARNING("MapObjectTypeBattalionMap has not been loaded.");
			return { };
		}

		const auto it = map_object_type_battalion_map->find(map_object_type_id);
		if(it == map_object_type_battalion_map->end()){
			LOG_EMPERY_CENTER_DEBUG("MapObjectTypeBattalion not found: map_object_type_id = ", map_object_type_id);
			return { };
		}
		return boost::shared_ptr<const MapObjectTypeBattalion>(map_object_type_battalion_map, &(it->second));
	}
	boost::shared_ptr<const MapObjectTypeBattalion> MapObjectTypeBattalion::require(MapObjectTypeId map_object_type_id){
		PROFILE_ME;

		auto ret = get(map_object_type_id);
		if(!ret){
			DEBUG_THROW(Exception, sslit("MapObjectTypeBattalion not found"));
		}
		return ret;
	}

	boost::shared_ptr<const MapObjectTypeMonster> MapObjectTypeMonster::get(MapObjectTypeId map_object_type_id){
		PROFILE_ME;

		const auto map_object_type_monster_map = g_map_object_type_monster_map.lock();
		if(!map_object_type_monster_map){
			LOG_EMPERY_CENTER_WARNING("MapObjectTypeMonsterMap has not been loaded.");
			return { };
		}

		const auto it = map_object_type_monster_map->find(map_object_type_id);
		if(it == map_object_type_monster_map->end()){
			LOG_EMPERY_CENTER_DEBUG("MapObjectTypeMonster not found: map_object_type_id = ", map_object_type_id);
			return { };
		}
		return boost::shared_ptr<const MapObjectTypeMonster>(map_object_type_monster_map, &(it->second));
	}
	boost::shared_ptr<const MapObjectTypeMonster> MapObjectTypeMonster::require(MapObjectTypeId map_object_type_id){
		PROFILE_ME;

		auto ret = get(map_object_type_id);
		if(!ret){
			DEBUG_THROW(Exception, sslit("MapObjectTypeMonster not found"));
		}
		return ret;
	}
}

}
