#ifndef EMPERY_CLUSTER_DATA_MAP_HPP_
#define EMPERY_CLUSTER_DATA_MAP_HPP_

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
	};

	class MapTerrain {
	public:
		static boost::shared_ptr<const MapTerrain> get(TerrainId terrain_id);
		static boost::shared_ptr<const MapTerrain> require(TerrainId terrain_id);

	public:
		TerrainId terrain_id;
		bool passable;
	};
}

}

#endif
