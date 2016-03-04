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
		std::uint64_t distance_to_hint;

		bool closed;
		std::uint64_t distance_from;
		Coord parent_coord;
	};

	bool compare_astar_coords_by_distance_hint(const AStarCoordElement &lhs, const AStarCoordElement &rhs){
		return lhs.distance_to_hint + lhs.distance_from > rhs.distance_to_hint + rhs.distance_from;
	}
}

bool find_path(std::vector<std::pair<signed char, signed char>> &path,
	Coord from_coord, Coord to_coord, AccountUuid account_uuid, std::uint64_t distance_limit, std::uint64_t distance_close_enough)
{
	PROFILE_ME;
	LOG_EMPERY_CLUSTER_DEBUG("Pathfinding: from_coord = ", from_coord, ", to_coord = ", to_coord,
		", distance_limit = ", distance_limit, ", distance_close_enough = ", distance_close_enough);

	if(from_coord == to_coord){
		return true;
	}

	boost::container::flat_map<Coord, AStarCoordElement> astar_coords;
	astar_coords.reserve(distance_limit * distance_limit * 4);

	const auto populate_coord = [&](Coord coord, bool closed_hint,
		std::uint64_t distance_from_hint, Coord parent_coord_hint) -> AStarCoordElement &
	{
		auto it = astar_coords.find(coord);
		if(it == astar_coords.end()){
			AStarCoordElement elem = { };
			elem.coord = coord;
			if(closed_hint){
				elem.closed = true;
			} else {
				const auto result = get_move_result(coord, account_uuid, true);
				elem.closed = (result.first != Msg::ST_OK) && (result.first != Msg::ERR_BLOCKED_BY_TROOPS_TEMPORARILY);
			}
			elem.distance_from = distance_from_hint;
			elem.parent_coord = parent_coord_hint;
			elem.distance_to_hint = get_distance_of_coords(coord, to_coord);
			it = astar_coords.emplace(coord, elem).first;
		} else {
			auto &elem = it->second;
			if(closed_hint){
				elem.closed = true;
			}
			if(elem.distance_from > distance_from_hint){
				elem.distance_from = distance_from_hint;
				elem.parent_coord = parent_coord_hint;
			}
		}
		return it->second;
	};

	std::vector<Coord> surrounding;

	std::vector<AStarCoordElement> coords_open;
	coords_open.reserve(distance_limit * 2);
	const auto &init_elem = populate_coord(from_coord, true /* 实际上不会影响功能 */, 0, from_coord);
	coords_open.emplace_back(init_elem);

	for(;;){
		// 获得距离总和最小的一点，然后把它从队列中删除。注意维护优先级。
		const auto elem_popped = coords_open.front();
		astar_coords.at(elem_popped.coord).closed = true;

		std::pop_heap(coords_open.begin(), coords_open.end(), &compare_astar_coords_by_distance_hint);
		coords_open.pop_back();

		// 展开之。
		if(elem_popped.distance_from < distance_limit){
			surrounding.clear();
			get_surrounding_coords(surrounding, elem_popped.coord, 1);
			for(auto it = surrounding.begin(); it != surrounding.end(); ++it){
				const auto coord = *it;
				const auto new_distance_from = elem_popped.distance_from + 1;
				const auto &new_elem = populate_coord(coord, false, new_distance_from, elem_popped.coord);
				if(new_elem.distance_to_hint <= distance_close_enough){
					// 寻路成功。
					std::deque<Coord> coord_queue;
					auto current_coord = coord;
					for(;;){
						coord_queue.emplace_front(current_coord);
						if(current_coord == from_coord){
							break;
						}
						current_coord = astar_coords.at(current_coord).parent_coord;
					}
					assert(!coord_queue.empty());
					path.reserve(path.size() + coord_queue.size() - 1);
					for(auto qit = coord_queue.begin() + 1; qit != coord_queue.end(); ++qit){
						path.emplace_back(qit[0].x() - qit[-1].x(), qit[0].y() - qit[-1].y());
					}
					return true;
				}
				if((new_distance_from < distance_limit) && !new_elem.closed){
					coords_open.emplace_back(new_elem);
					std::push_heap(coords_open.begin(), coords_open.end(), &compare_astar_coords_by_distance_hint);
				}
			}
		}

		if(coords_open.empty()){
			return false;
		}
	}
}

}
