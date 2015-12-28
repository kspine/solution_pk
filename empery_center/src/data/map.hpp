#ifndef EMPERY_CENTER_DATA_MAP_CELL_HPP_
#define EMPERY_CENTER_DATA_MAP_CELL_HPP_

#include "common.hpp"
#include <vector>

namespace EmperyCenter {

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

	class MapCellTicket {
	public:
		static boost::shared_ptr<const MapCellTicket> get(ItemId ticket_item_id);
		static boost::shared_ptr<const MapCellTicket> require(ItemId ticket_item_id);

	public:
		ItemId ticket_item_id;
		double production_rate_modifier;
		double capacity_modifier;
	};

	class MapTerrain {
	public:
		static boost::shared_ptr<const MapTerrain> get(TerrainId terrain_id);
		static boost::shared_ptr<const MapTerrain> require(TerrainId terrain_id);

	public:
		TerrainId terrain_id;
		ResourceId best_resource_id;
		double best_production_rate; // 每分钟产出资源。
		double best_capacity;
		bool buildable;
	};

	class MapOverlay {
	public:
		static boost::shared_ptr<const MapOverlay> get(OverlayId overlay_id);
		static boost::shared_ptr<const MapOverlay> require(OverlayId overlay_id);

	public:
		OverlayId overlay_id;
		ResourceId reward_resource_id;
		std::uint64_t reward_resource_amount;
	};

	class MapStartPoint {
	public:
		static boost::shared_ptr<const MapStartPoint> get(StartPointId start_point_id);
		static boost::shared_ptr<const MapStartPoint> require(StartPointId start_point_id);

		static void get_all(std::vector<boost::shared_ptr<const MapStartPoint>> &ret);

	public:
		StartPointId start_point_id;
		std::pair<unsigned, unsigned> map_coord;
	};
}

}

#endif
