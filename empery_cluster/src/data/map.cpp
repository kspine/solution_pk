#include "../precompiled.hpp"
#include "map.hpp"
#include <poseidon/multi_index_map.hpp>
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>

namespace EmperyCluster {

namespace {
	MULTI_INDEX_MAP(BasicMap, Data::MapCellBasic,
		UNIQUE_MEMBER_INDEX(map_coord)
		MULTI_MEMBER_INDEX(overlay_group_name)
	)
	boost::weak_ptr<const BasicMap> g_basic_map;
	const char BASIC_FILE[] = "map";

	MULTI_INDEX_MAP(TerrainMap, Data::MapCellTerrain,
		UNIQUE_MEMBER_INDEX(terrain_id)
	)
	boost::weak_ptr<const TerrainMap> g_terrain_map;
	const char TERRAIN_FILE[] = "Territory_product";

	MODULE_RAII_PRIORITY(handles, 1000){
		auto csv = Data::sync_load_data(BASIC_FILE);
		const auto basic_map = boost::make_shared<BasicMap>();
		while(csv.fetch_row()){
			Data::MapCellBasic elem = { };

			Poseidon::JsonArray array;
			csv.get(array, "xy");
			const unsigned x = array.at(0).get<double>();
			const unsigned y = array.at(1).get<double>();
			elem.map_coord = std::make_pair(x, y);

			csv.get(elem.terrain_id,         "property_id");
			csv.get(elem.overlay_id,         "remove_id");
			csv.get(elem.overlay_group_name, "group_id");

			if(!basic_map->insert(std::move(elem)).second){
				LOG_EMPERY_CLUSTER_ERROR("Duplicate MapCellBasic: x = ", elem.map_coord.first, ", y = ", elem.map_coord.second);
				DEBUG_THROW(Exception, sslit("Duplicate MapCellBasic"));
			}
		}
		g_basic_map = basic_map;
		handles.push(basic_map);

		csv = Data::sync_load_data(TERRAIN_FILE);
		const auto terrain_map = boost::make_shared<TerrainMap>();
		while(csv.fetch_row()){
			Data::MapCellTerrain elem = { };

			csv.get(elem.terrain_id,       "territory_id");

			csv.get(elem.passable,         "mobile");

			if(!terrain_map->insert(std::move(elem)).second){
				LOG_EMPERY_CLUSTER_ERROR("Duplicate MapCellTerrain: terrain_id = ", elem.terrain_id);
				DEBUG_THROW(Exception, sslit("Duplicate MapCellTerrain"));
			}
		}
		g_terrain_map = terrain_map;
		handles.push(terrain_map);
	}
}

namespace Data {
	boost::shared_ptr<const MapCellBasic> MapCellBasic::get(unsigned map_x, unsigned map_y){
		PROFILE_ME;

		const auto basic_map = g_basic_map.lock();
		if(!basic_map){
			LOG_EMPERY_CLUSTER_WARNING("MapCellBasicMap has not been loaded.");
			return { };
		}

		const auto it = basic_map->find<0>(std::make_pair(map_x, map_y));
		if(it == basic_map->end<0>()){
			LOG_EMPERY_CLUSTER_DEBUG("MapCellterrain not found: map_x = ", map_x, ", map_y = ", map_y);
			return { };
		}
		return boost::shared_ptr<const MapCellBasic>(basic_map, &*it);
	}
	boost::shared_ptr<const MapCellBasic> MapCellBasic::require(unsigned map_x, unsigned map_y){
		PROFILE_ME;

		auto ret = get(map_x, map_y);
		if(!ret){
			DEBUG_THROW(Exception, sslit("MapCellBasic not found"));
		}
		return ret;
	}

	void MapCellBasic::get_by_overlay_group(std::vector<boost::shared_ptr<const MapCellBasic>> &ret, const std::string &overlay_group_name){
		PROFILE_ME;

		const auto basic_map = g_basic_map.lock();
		if(!basic_map){
			LOG_EMPERY_CLUSTER_WARNING("MapCellBasicMap has not been loaded.");
			return;
		}

		const auto range = basic_map->equal_range<1>(overlay_group_name);
		ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
		for(auto it = range.first; it != range.second; ++it){
			ret.emplace_back(basic_map, &*it);
		}
	}

	boost::shared_ptr<const MapCellTerrain> MapCellTerrain::get(TerrainId terrain_id){
		PROFILE_ME;

		const auto terrain_map = g_terrain_map.lock();
		if(!terrain_map){
			LOG_EMPERY_CLUSTER_WARNING("MapCellTerrainMap has not been loaded.");
			return { };
		}

		const auto it = terrain_map->find<0>(terrain_id);
		if(it == terrain_map->end<0>()){
			LOG_EMPERY_CLUSTER_DEBUG("MapCellTerrain not found: terrain_id = ", terrain_id);
			return { };
		}
		return boost::shared_ptr<const MapCellTerrain>(terrain_map, &*it);
	}
	boost::shared_ptr<const MapCellTerrain> MapCellTerrain::require(TerrainId terrain_id){
		PROFILE_ME;

		auto ret = get(terrain_id);
		if(!ret){
			DEBUG_THROW(Exception, sslit("MapCellTerrain not found"));
		}
		return ret;
	}
}

}

