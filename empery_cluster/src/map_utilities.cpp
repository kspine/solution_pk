#include "precompiled.hpp"
#include "map_utilities.hpp"

#include "../../empery_center/src/map_utilities.cpp"

#include "cluster_client.hpp"
#include "singletons/world_map.hpp"
#include "map_cell.hpp"
#include "map_object.hpp"
#include "map_object_type_ids.hpp"
#include "data/map.hpp"
#include "data/global.hpp"
#include "../../empery_center/src/msg/err_map.hpp"
#include "../../empery_center/src/cbpp_response.hpp"

namespace EmperyCluster {

namespace Msg {
	using namespace ::EmperyCenter::Msg;
}

using Response = ::EmperyCenter::CbppResponse;

std::pair<long, std::string> get_move_result(Coord coord, AccountUuid account_uuid, bool wait_for_moving_objects){
	PROFILE_ME;

	// 检测阻挡。
	const auto map_cell = WorldMap::get_map_cell(coord);
	if(map_cell){
		const auto cell_owner_uuid = map_cell->get_owner_uuid();
		if(cell_owner_uuid && (account_uuid != cell_owner_uuid)){
			LOG_EMPERY_CLUSTER_TRACE("Blocked by a cell owned by another player's territory: cell_owner_uuid = ", cell_owner_uuid);
			return Response(Msg::ERR_BLOCKED_BY_OTHER_TERRITORY) <<cell_owner_uuid;
		}
	}
/*
	const auto cluster = WorldMap::get_cluster(coord);
	if(!cluster){
		LOG_EMPERY_CLUSTER_TRACE("Lost connection to center server: coord = ", coord);
		return Response(Msg::ERR_CLUSTER_CONNECTION_LOST) <<coord;
	}
*/
	const auto cluster_scope = WorldMap::get_cluster_scope(coord);
	const auto map_x = static_cast<unsigned>(coord.x() - cluster_scope.left());
	const auto map_y = static_cast<unsigned>(coord.y() - cluster_scope.bottom());
	const auto cell_data = Data::MapCellBasic::require(map_x, map_y);
	const auto terrain_id = cell_data->terrain_id;
	const auto terrain_data = Data::MapTerrain::require(terrain_id);
	if(!terrain_data->passable){
		LOG_EMPERY_CLUSTER_TRACE("Blocked by terrain: terrain_id = ", terrain_id);
		return Response(Msg::ERR_BLOCKED_BY_IMPASSABLE_MAP_CELL) <<terrain_id;
	}
	const unsigned border_thickness = Data::Global::as_unsigned(Data::Global::SLOT_MAP_BORDER_THICKNESS);
	if((map_x < border_thickness) || (map_x >= cluster_scope.width() - border_thickness) ||
		(map_y < border_thickness) || (map_y >= cluster_scope.height() - border_thickness))
	{
		LOG_EMPERY_CLUSTER_TRACE("Blocked by map border: coord = ", coord);
		return Response(Msg::ERR_BLOCKED_BY_IMPASSABLE_MAP_CELL) <<coord;
	}

	std::vector<boost::shared_ptr<MapObject>> adjacent_objects;
	WorldMap::get_map_objects_by_rectangle(adjacent_objects,
		Rectangle(Coord(coord.x() - 3, coord.y() - 3), Coord(coord.x() + 4, coord.y() + 4)));
	std::vector<Coord> foundation;
	for(auto it = adjacent_objects.begin(); it != adjacent_objects.end(); ++it){
		const auto &other_object = *it;
		const auto other_map_object_uuid = other_object->get_map_object_uuid();
		const auto other_coord = other_object->get_coord();
		if(coord == other_coord){
			LOG_EMPERY_CLUSTER_TRACE("Blocked by another map object: other_map_object_uuid = ", other_map_object_uuid);
			if(wait_for_moving_objects && other_object->is_moving()){
				return Response(Msg::ERR_BLOCKED_BY_TROOPS_TEMPORARILY) <<other_map_object_uuid;
			}
			return Response(Msg::ERR_BLOCKED_BY_TROOPS) <<other_map_object_uuid;
		}
		const auto other_account_uuid = other_object->get_owner_uuid();
		const auto other_object_type_id = other_object->get_map_object_type_id();
		if((other_account_uuid != account_uuid) && (other_object_type_id == MapObjectTypeIds::ID_CASTLE)){
			foundation.clear();
			get_castle_foundation(foundation, other_coord, false);
			for(auto fit = foundation.begin(); fit != foundation.end(); ++fit){
				if(coord == *fit){
					LOG_EMPERY_CLUSTER_TRACE("Blocked by castle: other_map_object_uuid = ", other_map_object_uuid);
					return Response(Msg::ERR_BLOCKED_BY_CASTLE) <<other_map_object_uuid;
				}
			}
		}
	}

	return Response();
}

namespace {
	struct AStarCoordElement {
		Coord coord;
		bool closed;
		Coord parent_coord;
		std::uint64_t distance_from;
		std::uint64_t distance_to;
	};

	bool operator<(const AStarCoordElement &lhs, const AStarCoordElement &rhs){
		return lhs.distance_from + lhs.distance_to > rhs.distance_to + rhs.distance_to;
	}
}

bool find_path(std::vector<std::pair<signed char, signed char>> &path,
	Coord from_coord, Coord to_coord, AccountUuid account_uuid, std::uint64_t radius_limit, std::uint64_t distance_close_enough)
{
	PROFILE_ME;
	LOG_EMPERY_CLUSTER_DEBUG("Pathfinding: from_coord = ", from_coord, ", to_coord = ", to_coord,
		", radius_limit = ", radius_limit, ", distance_close_enough = ", distance_close_enough);

	if(from_coord == to_coord){
		return true;
	}

	boost::container::flat_map<Coord, AStarCoordElement> astar_coords;
	astar_coords.reserve(radius_limit * radius_limit * 4);

	const auto populate_coord = [&](Coord coord, bool closed_hint) -> AStarCoordElement & {
		auto it = astar_coords.find(coord);
		if(it == astar_coords.end()){
			AStarCoordElement elem = { };
			elem.coord         = coord;
			if(closed_hint){
				elem.closed = true;
			} else {
				const auto result = get_result(coord, account_uuid, true);
				elem.closed = (result.first != Msg::ST_OK) && (result.first != Msg::ERR_BLOCKED_BY_TROOPS_TEMPORARILY);
			}
			elem.distance_from = get_distance(from_coord, coord);
			elem.distance_to   = get_distance(coord, to_coord);
			it = astar_coords.emplace(from_coord, elem).first;
		} else {
			if(closed_hint){
				it->second.closed = true;
			}
		}
		return it->second;
	};

	std::vector<Coord> coords_open;
	coords_open.reserve(radius_limit * 2);

	AStarCoordElement elem = { };
	elem.coord         = from_coord;
	elem.closed        = false;
	elem.distance_from = 0;
	elem.distance_to   = get_distance(from_coord, to_coord);
	astar_coords.emplace(from_coord, elem);
	coords_open.emplace_back(elem);

	std::vector<Coord> surrounding;

	for(;;){
		if(coords_open.empty()){
			return false;
		}
		// 获得距离总和最小的一点。
		const auto coord_popped = coords_open.front().coord;
		
		std::pop_heap(coords_open.begin(), coords_open.end());
		coords_open.pop_back();

		// 展开之。
		surrounding.clear();
		get_surrounding_coords(surrounding, elem_popped.coord, 1);

		
	}
}

}
#include <poseidon/singletons/timer_daemon.hpp>
#include "mmain.hpp"
MODULE_RAII(handles){
	using namespace EmperyCluster;
	handles.push(Poseidon::TimerDaemon::register_timer(10000, 0,
		std::bind([]{
			std::vector<std::pair<signed char, signed char>> path;
			const bool ret = find_path(path, Coord(-207,624), Coord(-206,621), AccountUuid(), 20, 0);
			LOG_EMPERY_CLUSTER_FATAL("Pathfinding result = ", ret);
			for(auto it = path.begin(); it != path.end(); ++it){
				LOG_EMPERY_CLUSTER_FATAL("> Path: dx = ", it->first, ", dy = ", it->second);
			}
		})
	));
}
