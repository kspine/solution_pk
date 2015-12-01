#ifndef EMPERY_CENTER_DATA_MAP_CELL_HPP_
#define EMPERY_CENTER_DATA_MAP_CELL_HPP_

#include "common.hpp"

namespace EmperyCenter {

namespace Data {
	class MapCell {
	public:
		static boost::shared_ptr<const MapCell> get(boost::uint32_t map_x, boost::uint32_t map_y);
		static boost::shared_ptr<const MapCell> require(boost::uint32_t map_x, boost::uint32_t map_y);

	public:
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

	class MapCellProduction {
	public:
		static boost::shared_ptr<const MapCellProduction> get(TerrainId terrain_id);
		static boost::shared_ptr<const MapCellProduction> require(TerrainId terrain_id);

	public:
		TerrainId terrain_id;
		ResourceId best_resource_id;
		double best_production_rate; // 每毫秒产出资源。
		double best_capacity;
		bool passable;
	};
}

}

#endif
