#ifndef EMPERY_CLUSTER_DATA_MAP_CELL_HPP_
#define EMPERY_CLUSTER_DATA_MAP_CELL_HPP_

#include "common.hpp"
#include <array>

namespace EmperyCluster {

namespace Data {
	class MapCellBasic {
	public:
		static boost::shared_ptr<const MapCellBasic> get(unsigned map_x, unsigned map_y);
		static boost::shared_ptr<const MapCellBasic> require(unsigned map_x, unsigned map_y);

	public:
		std::pair<unsigned, unsigned> map_coord;
		TerrainId terrain_id;
		OverlayId overlay_id;
		std::array<char, 32> group;
	};

	class MapCellTerrain {
	public:
		static boost::shared_ptr<const MapCellTerrain> get(TerrainId terrain_id);
		static boost::shared_ptr<const MapCellTerrain> require(TerrainId terrain_id);

	public:
		TerrainId terrain_id;
		bool passable;
	};
}

}

#endif
