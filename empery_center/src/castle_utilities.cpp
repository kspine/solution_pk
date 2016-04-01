#include "precompiled.hpp"
#include "castle_utilities.hpp"
#include "map_utilities.hpp"
#include "cbpp_response.hpp"
#include "data/global.hpp"
#include "data/map.hpp"
#include "map_cell.hpp"
#include "overlay.hpp"
#include "map_object.hpp"
#include "strategic_resource.hpp"
#include "singletons/world_map.hpp"
#include "msg/err_map.hpp"
#include "map_object_type_ids.hpp"

namespace EmperyCenter {

std::pair<long, std::string> can_deploy_castle_at(Coord coord, MapObjectUuid excluding_map_object_uuid){
	PROFILE_ME;

	using Response = CbppResponse;

	std::vector<boost::shared_ptr<MapCell>> map_cells;
	std::vector<boost::shared_ptr<Overlay>> overlays;
	std::vector<boost::shared_ptr<MapObject>> map_objects;
	std::vector<boost::shared_ptr<StrategicResource>> strategic_resources;

	std::vector<Coord> foundation;
	get_castle_foundation(foundation, coord, true);
	for(auto it = foundation.begin(); it != foundation.end(); ++it){
		const auto &foundation_coord = *it;
		const auto cluster_scope = WorldMap::get_cluster_scope(foundation_coord);
		const auto map_x = static_cast<unsigned>(foundation_coord.x() - cluster_scope.left());
		const auto map_y = static_cast<unsigned>(foundation_coord.y() - cluster_scope.bottom());
		LOG_EMPERY_CENTER_DEBUG("Castle foundation: foundation_coord = ", foundation_coord, ", cluster_scope = ", cluster_scope,
			", map_x = ", map_x, ", map_y = ", map_y);
		const auto basic_data = Data::MapCellBasic::require(map_x, map_y);
		const auto terrain_data = Data::MapTerrain::require(basic_data->terrain_id);
		if(!terrain_data->buildable){
			return Response(Msg::ERR_CANNOT_DEPLOY_IMMIGRANTS_HERE) <<foundation_coord;
		}

		map_cells.clear();
		WorldMap::get_map_cells_by_rectangle(map_cells, Rectangle(foundation_coord, 1, 1));
		for(auto it = map_cells.begin(); it != map_cells.end(); ++it){
			const auto &map_cell = *it;
			if(!map_cell->is_virtually_removed()){
				return Response(Msg::ERR_CANNOT_DEPLOY_ON_TERRITORY) <<foundation_coord;
			}
		}

		overlays.clear();
		WorldMap::get_overlays_by_rectangle(overlays, Rectangle(foundation_coord, 1, 1));
		for(auto it = overlays.begin(); it != overlays.end(); ++it){
			const auto &overlay = *it;
			if(!overlay->is_virtually_removed()){
				return Response(Msg::ERR_CANNOT_DEPLOY_ON_OVERLAY) <<foundation_coord;
			}
		}

		map_objects.clear();
		WorldMap::get_map_objects_by_rectangle(map_objects, Rectangle(foundation_coord, 1, 1));
		for(auto it = map_objects.begin(); it != map_objects.end(); ++it){
			const auto &other_object = *it;
			if(other_object->get_map_object_uuid() == excluding_map_object_uuid){
				continue;
			}
			if(!other_object->is_virtually_removed()){
				return Response(Msg::ERR_CANNOT_DEPLOY_ON_TROOPS) <<foundation_coord;
			}
		}

		strategic_resources.clear();
		WorldMap::get_strategic_resources_by_rectangle(strategic_resources, Rectangle(foundation_coord, 1, 1));
		for(auto it = strategic_resources.begin(); it != strategic_resources.end(); ++it){
			const auto &strategic_resource = *it;
			if(!strategic_resource->is_virtually_removed()){
				return Response(Msg::ERR_CANNOT_DEPLOY_ON_STRATEGIC_RESOURCE) <<foundation_coord;
			}
		}
	}
	// 检测与其他城堡距离。
	const auto min_distance  = static_cast<std::uint32_t>(Data::Global::as_unsigned(Data::Global::SLOT_MINIMUM_DISTANCE_BETWEEN_CASTLES));

	const auto cluster_scope = WorldMap::get_cluster_scope(coord);
	const auto other_left    = std::max(coord.x() - (min_distance - 1), cluster_scope.left());
	const auto other_bottom  = std::max(coord.y() - (min_distance - 1), cluster_scope.bottom());
	const auto other_right   = std::min(coord.x() + (min_distance + 2), cluster_scope.right());
	const auto other_top     = std::min(coord.y() + (min_distance + 2), cluster_scope.top());
	map_objects.clear();
	WorldMap::get_map_objects_by_rectangle(map_objects, Rectangle(Coord(other_left, other_bottom), Coord(other_right, other_top)));
	for(auto it = map_objects.begin(); it != map_objects.end(); ++it){
		const auto &other_object = *it;
		const auto other_object_type_id = other_object->get_map_object_type_id();
		if(other_object_type_id != MapObjectTypeIds::ID_CASTLE){
			continue;
		}
		const auto other_coord = other_object->get_coord();
		const auto other_object_uuid = other_object->get_map_object_uuid();
		LOG_EMPERY_CENTER_DEBUG("Checking distance: other_coord = ", other_coord, ", other_object_uuid = ", other_object_uuid);
		const auto distance = get_distance_of_coords(other_coord, coord);
		if(distance <= min_distance){
			return Response(Msg::ERR_TOO_CLOSE_TO_ANOTHER_CASTLE) <<other_object_uuid;
		}
	}

	return Response();
}

}
