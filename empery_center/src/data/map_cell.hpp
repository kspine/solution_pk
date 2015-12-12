#ifndef EMPERY_CENTER_DATA_MAP_CELL_HPP_
#define EMPERY_CENTER_DATA_MAP_CELL_HPP_

#include "common.hpp"

namespace EmperyCenter {

namespace Data {
	class MapCellBasic {
	public:
		static boost::shared_ptr<const MapCellBasic> get(unsigned map_x, unsigned map_y);
		static boost::shared_ptr<const MapCellBasic> require(unsigned map_x, unsigned map_y);

	public:
		std::pair<unsigned, unsigned> map_coord;
		TerrainId terrain_id;
	};

	class MapCellTicket {
	public:
		static boost::shared_ptr<const MapCellTicket> get(ItemId ticket_item_id);
		static boost::shared_ptr<const MapCellTicket> require(ItemId ticket_item_id);

	public:
		ItemId ticket_item_id;
		double production_rate_modifier;
		double capacity_modifier;
	};

	class MapCellTerrain {
	public:
		static boost::shared_ptr<const MapCellTerrain> get(TerrainId terrain_id);
		static boost::shared_ptr<const MapCellTerrain> require(TerrainId terrain_id);

	public:
		TerrainId terrain_id;
		ResourceId best_resource_id;
		double best_production_rate; // 每毫秒产出资源。
		double best_capacity;
		bool buildable;
	};
}

}

#endif
