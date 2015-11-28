#ifndef EMPERY_CENTER_DATA_MAP_CELL_HPP_
#define EMPERY_CENTER_DATA_MAP_CELL_HPP_

#include "common.hpp"

namespace EmperyCenter {

namespace Data {
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

		static bool is_resource_producible(ResourceId resource_id);

	public:
		TerrainId terrain_id;
		ResourceId best_resource_id;
		boost::uint64_t best_production_rate;
		boost::uint64_t best_capacity;
		bool passable;
	};
}

}

#endif
