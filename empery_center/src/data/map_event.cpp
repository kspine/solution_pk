#include "../precompiled.hpp"
#include "map_event.hpp"
#include <poseidon/multi_index_map.hpp>
#include <string.h>
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>
#include "../data_session.hpp"

namespace EmperyCenter {

namespace {
	MULTI_INDEX_MAP(MapEventBlockMap, Data::MapEventBlock,
		UNIQUE_MEMBER_INDEX(num_coord)
	)
	boost::weak_ptr<const MapEventBlockMap> g_map_event_block_map;
	const char MAP_EVENT_BLOCK_FILE[] = "event_circle";

	MULTI_INDEX_MAP(MapEventGenerationMap, Data::MapEventGeneration,
		UNIQUE_MEMBER_INDEX(unique_id)
		MULTI_MEMBER_INDEX(map_event_circle_id)
	)
	boost::weak_ptr<const MapEventGenerationMap> g_map_event_generation_map;
	const char MAP_EVENT_GENERATION_FILE[] = "event_fresh";

	using MapEventResourceMap = boost::container::flat_map<MapEventId, Data::MapEventResource>;
	boost::weak_ptr<const MapEventResourceMap> g_map_event_resource_map;
	const char MAP_EVENT_RESOURCE_FILE[] = "event_resource";

	using MapEventMonsterMap = boost::container::flat_map<MapEventId, Data::MapEventMonster>;
	boost::weak_ptr<const MapEventMonsterMap> g_map_event_monster_map;
	const char MAP_EVENT_MONSTER_FILE[] = "event_monster";

	template<typename ElementT>
	void read_map_event_abstract(ElementT &elem, const Poseidon::CsvParser &csv){
		csv.get(elem.map_event_id,          "event_id");
		csv.get(elem.restricted_terrain_id, "land_id");
	}

	MODULE_RAII_PRIORITY(handles, 1000){
		auto csv = Data::sync_load_data(MAP_EVENT_BLOCK_FILE);
		const auto map_event_block_map = boost::make_shared<MapEventBlockMap>();
		while(csv.fetch_row()){
			Data::MapEventBlock elem = { };

			Poseidon::JsonArray array;
			csv.get(array, "refresh_circle");
			elem.num_coord.first  = array.at(0).get<double>();
			elem.num_coord.second = array.at(1).get<double>();

			csv.get(elem.map_event_circle_id, "circle_id");
			csv.get(elem.refresh_interval,    "refresh_time");
			csv.get(elem.priority,            "precedence");

			if(!map_event_block_map->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate MapEventBlock: num_coord = ", elem.num_coord.first, ", ", elem.num_coord.second);
				DEBUG_THROW(Exception, sslit("Duplicate MapEventBlock"));
			}
		}
		g_map_event_block_map = map_event_block_map;
		handles.push(map_event_block_map);
		auto servlet = DataSession::create_servlet(MAP_EVENT_BLOCK_FILE, Data::encode_csv_as_json(csv, "refresh_circle"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(MAP_EVENT_GENERATION_FILE);
		const auto map_event_generation_map = boost::make_shared<MapEventGenerationMap>();
		while(csv.fetch_row()){
			Data::MapEventGeneration elem = { };

			csv.get(elem.unique_id,              "id");
			csv.get(elem.map_event_circle_id,    "resource_circle");
			csv.get(elem.map_event_id,           "event_id");
			csv.get(elem.event_count_multiplier, "event_quantity");
			csv.get(elem.expiry_duration,        "event_active_time");
			csv.get(elem.priority,               "priority");

			if(!map_event_generation_map->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate MapEventGeneration: unique_id = ", elem.unique_id);
				DEBUG_THROW(Exception, sslit("Duplicate MapEventGeneration"));
			}
		}
		g_map_event_generation_map = map_event_generation_map;
		handles.push(map_event_generation_map);
		servlet = DataSession::create_servlet(MAP_EVENT_GENERATION_FILE, Data::encode_csv_as_json(csv, "id"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(MAP_EVENT_RESOURCE_FILE);
		const auto map_event_resource_map = boost::make_shared<MapEventResourceMap>();
		while(csv.fetch_row()){
			Data::MapEventResource elem = { };

			read_map_event_abstract(elem, csv);

			csv.get(elem.resource_id,     "resource_id");
			csv.get(elem.resource_amount, "resource_number");

			if(!map_event_resource_map->emplace(elem.map_event_id, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate MapEventResource: map_event_id = ", elem.map_event_id);
				DEBUG_THROW(Exception, sslit("Duplicate MapEventResource"));
			}
		}
		g_map_event_resource_map = map_event_resource_map;
		handles.push(map_event_resource_map);
		servlet = DataSession::create_servlet(MAP_EVENT_RESOURCE_FILE, Data::encode_csv_as_json(csv, "event_id"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(MAP_EVENT_MONSTER_FILE);
		const auto map_event_monster_map = boost::make_shared<MapEventMonsterMap>();
		while(csv.fetch_row()){
			Data::MapEventMonster elem = { };

			read_map_event_abstract(elem, csv);

			csv.get(elem.monster_type_id, "event_monster");

			if(!map_event_monster_map->emplace(elem.map_event_id, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate MapEventMonster: map_event_id = ", elem.map_event_id);
				DEBUG_THROW(Exception, sslit("Duplicate MapEventMonster"));
			}
		}
		g_map_event_monster_map = map_event_monster_map;
		handles.push(map_event_monster_map);
		servlet = DataSession::create_servlet(MAP_EVENT_MONSTER_FILE, Data::encode_csv_as_json(csv, "event_id"));
		handles.push(std::move(servlet));
	}
}

namespace Data {
	boost::shared_ptr<const MapEventBlock> MapEventBlock::get(unsigned num_x, unsigned num_y){
		PROFILE_ME;

		const auto map_event_block_map = g_map_event_block_map.lock();
		if(!map_event_block_map){
			LOG_EMPERY_CENTER_WARNING("MapEventBlockMap has not been loaded.");
			return { };
		}

		const auto it = map_event_block_map->find<0>(std::make_pair(num_x, num_y));
		if(it == map_event_block_map->end<0>()){
			LOG_EMPERY_CENTER_TRACE("MapEventBlock not found: num_x = ", num_x, ", num_y = ", num_y);
			return { };
		}
		return boost::shared_ptr<const MapEventBlock>(map_event_block_map, &*it);
	}
	boost::shared_ptr<const MapEventBlock> MapEventBlock::require(unsigned num_x, unsigned num_y){
		PROFILE_ME;

		auto ret = get(num_x, num_y);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("MapEventBlock not found: num_x = ", num_x, ", num_y = ", num_y);
			DEBUG_THROW(Exception, sslit("MapEventBlock not found"));
		}
		return ret;
	}

	void MapEventBlock::get_all(std::vector<boost::shared_ptr<const MapEventBlock>> &ret){
		PROFILE_ME;

		const auto map_event_block_map = g_map_event_block_map.lock();
		if(!map_event_block_map){
			LOG_EMPERY_CENTER_WARNING("MapEventBlockMap has not been loaded.");
			return;
		}

		ret.reserve(ret.size() + map_event_block_map->size());
		for(auto it = map_event_block_map->begin<0>(); it != map_event_block_map->end<0>(); ++it){
			ret.emplace_back(map_event_block_map, &*it);
		}
	}

	boost::shared_ptr<const MapEventGeneration> MapEventGeneration::get(std::uint64_t unique_id){
		PROFILE_ME;

		const auto map_event_generation_map = g_map_event_generation_map.lock();
		if(!map_event_generation_map){
			LOG_EMPERY_CENTER_WARNING("MapEventGenerationMap has not been loaded.");
			return { };
		}

		const auto it = map_event_generation_map->find<0>(unique_id);
		if(it == map_event_generation_map->end<0>()){
			LOG_EMPERY_CENTER_TRACE("MapEventGeneration not found: unique_id = ", unique_id);
			return { };
		}
		return boost::shared_ptr<const MapEventGeneration>(map_event_generation_map, &*it);
	}
	boost::shared_ptr<const MapEventGeneration> MapEventGeneration::require(std::uint64_t unique_id){
		PROFILE_ME;

		auto ret = get(unique_id);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("MapEventGeneration not found: unique_id = ", unique_id);
			DEBUG_THROW(Exception, sslit("MapEventGeneration not found"));
		}
		return ret;
	}

	void MapEventGeneration::get_by_map_event_circle_id(std::vector<boost::shared_ptr<const MapEventGeneration>> &ret,
		MapEventCircleId map_event_circle_id)
	{
		PROFILE_ME;

		const auto map_event_generation_map = g_map_event_generation_map.lock();
		if(!map_event_generation_map){
			LOG_EMPERY_CENTER_WARNING("MapEventGenerationMap has not been loaded.");
			return;
		}

		const auto range = map_event_generation_map->equal_range<1>(map_event_circle_id);
		ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
		for(auto it = range.first; it != range.second; ++it){
			ret.emplace_back(map_event_generation_map, &*it);
		}
	}

	boost::shared_ptr<const MapEventAbstract> MapEventAbstract::get(MapEventId map_event_id){
		PROFILE_ME;

		boost::shared_ptr<const MapEventAbstract> ret;
		if(!!(ret = MapEventResource::get(map_event_id))){
			return ret;
		}
		if(!!(ret = MapEventMonster::get(map_event_id))){
			return ret;
		}
		return ret;
	}
	boost::shared_ptr<const MapEventAbstract> MapEventAbstract::require(MapEventId map_event_id){
		PROFILE_ME;

		auto ret = get(map_event_id);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("MapEventAbstract not found: map_event_id = ", map_event_id);
			DEBUG_THROW(Exception, sslit("MapEventAbstract not found"));
		}
		return ret;
	}

	boost::shared_ptr<const MapEventResource> MapEventResource::get(MapEventId map_event_id){
		PROFILE_ME;

		const auto map_event_resource_map = g_map_event_resource_map.lock();
		if(!map_event_resource_map){
			LOG_EMPERY_CENTER_WARNING("MapEventResourceMap has not been loaded.");
			return { };
		}

		const auto it = map_event_resource_map->find(map_event_id);
		if(it == map_event_resource_map->end()){
			LOG_EMPERY_CENTER_TRACE("MapEventResource not found: map_event_id = ", map_event_id);
			return { };
		}
		return boost::shared_ptr<const MapEventResource>(map_event_resource_map, &it->second);
	}
	boost::shared_ptr<const MapEventResource> MapEventResource::require(MapEventId map_event_id){
		PROFILE_ME;

		auto ret = get(map_event_id);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("MapEventResource not found: map_event_id = ", map_event_id);
			DEBUG_THROW(Exception, sslit("MapEventResource not found"));
		}
		return ret;
	}

	boost::shared_ptr<const MapEventMonster> MapEventMonster::get(MapEventId map_event_id){
		PROFILE_ME;

		const auto map_event_monster_map = g_map_event_monster_map.lock();
		if(!map_event_monster_map){
			LOG_EMPERY_CENTER_WARNING("MapEventMonsterMap has not been loaded.");
			return { };
		}

		const auto it = map_event_monster_map->find(map_event_id);
		if(it == map_event_monster_map->end()){
			LOG_EMPERY_CENTER_TRACE("MapEventMonster not found: map_event_id = ", map_event_id);
			return { };
		}
		return boost::shared_ptr<const MapEventMonster>(map_event_monster_map, &it->second);
	}
	boost::shared_ptr<const MapEventMonster> MapEventMonster::require(MapEventId map_event_id){
		PROFILE_ME;

		auto ret = get(map_event_id);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("MapEventMonster not found: map_event_id = ", map_event_id);
			DEBUG_THROW(Exception, sslit("MapEventMonster not found"));
		}
		return ret;
	}
}

}
