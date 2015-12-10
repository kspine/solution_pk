#include "../precompiled.hpp"
#include "map_cell.hpp"
#include <poseidon/multi_index_map.hpp>

namespace EmperyCluster {

namespace {
	MULTI_INDEX_MAP(BasicMap, Data::MapCellBasic,
		UNIQUE_MEMBER_INDEX(map_coord)
	)
	boost::weak_ptr<const BasicMap> g_basic_map;
	const char BASIC_FILE[] = "map";

	MULTI_INDEX_MAP(TerrainMap, Data::MapCellTerrain,
		UNIQUE_MEMBER_INDEX(terrain_id)
	)
	boost::weak_ptr<const TerrainMap> g_terrain_map;
	const char TERRAIN_FILE[] = "Territory_product";

	MODULE_RAII_PRIORITY(handles, 1000){
//	extern std::vector<Poseidon::OptionalMap> sync_get_data_from_CLUSTER(const char *path);
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

