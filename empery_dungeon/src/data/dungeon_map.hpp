#ifndef EMPERY_DUNGEON_DATA_DUNGEON_MAP_CELL_HPP_
#define EMPERY_DUNGEON_DATA_DUNGEON_MAP_CELL_HPP_

#include "common.hpp"
#include <vector>
#include <boost/container/flat_map.hpp>

namespace EmperyDungeon {

namespace Data {
	class MapCellBasic {
	public:
		static boost::shared_ptr<const MapCellBasic> get(std::string dungeon_map_name,int map_x, int map_y);
		static boost::shared_ptr<const MapCellBasic> require(std::string dungeon_map_name,int map_x, int map_y);

	public:
		std::pair<std::string,std::pair<int, int>>  dungeon_map_coord;
		TerrainId terrain_id;
	};
	
	class MapTerrain {
	public:
		static boost::shared_ptr<const MapTerrain> get(TerrainId terrain_id);
		static boost::shared_ptr<const MapTerrain> require(TerrainId terrain_id);

	public:
		TerrainId terrain_id;
		ResourceId best_resource_id;
		double best_production_rate; // 每分钟产出资源。
		std::uint64_t best_capacity;
		bool buildable;
		bool passable;
		std::uint64_t protection_cost;
	};
}

}

#endif
