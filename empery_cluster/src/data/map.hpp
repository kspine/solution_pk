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

		static void get_by_overlay_group(std::vector<boost::shared_ptr<const MapCellBasic>> &ret, const std::string &overlay_group_name);

	public:
		std::pair<unsigned, unsigned> map_coord;
		TerrainId terrain_id;
		OverlayId overlay_id;
		std::string overlay_group_name;
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
