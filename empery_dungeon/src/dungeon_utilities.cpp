#include "precompiled.hpp"
#include "dungeon_utilities.hpp"
#include "dungeon_client.hpp"
#include "dungeon_object.hpp"
#include "data/dungeon_map.hpp"
#include "data/dungeon.hpp"
#include "cbpp_response.hpp"
#include "singletons/dungeon_map.hpp"
#include "../../empery_center/src/map_utilities.cpp"
#include "../../empery_center/src/msg/err_map.hpp"
#include "../../empery_center/src/msg/err_dungeon.hpp"


namespace EmperyDungeon {

namespace Msg = ::EmperyCenter::Msg;

std::pair<long, std::string> get_move_result(DungeonUuid dungeon_uuid,Coord coord, AccountUuid account_uuid, bool wait_for_moving_objects){
	PROFILE_ME;
	const auto dungeon = DungeonMap::require(dungeon_uuid);
	const auto dungeon_data = Data::Dungeon::require(dungeon->get_dungeon_type_id());
	const auto map_x = coord.x();
	const auto map_y = coord.y();
	const auto cell_data = Data::MapCellBasic::get(dungeon_data->dungeon_map,map_x, map_y);
	if(!cell_data){
		return CbppResponse(Msg::ERR_NO_DUNGEON_CELL) << dungeon_data->dungeon_map << map_x << map_y;
	}
	const auto terrain_id = cell_data->terrain_id;
	const auto terrain_data = Data::MapTerrain::require(terrain_id);
	if(!terrain_data->passable){
		LOG_EMPERY_DUNGEON_TRACE("Blocked by terrain: terrain_id = ", terrain_id);
		return CbppResponse(Msg::ERR_BLOCKED_BY_IMPASSABLE_MAP_CELL) <<terrain_id;
	}
	if(dungeon->is_dungeon_blocks_coord(coord)){
		LOG_EMPERY_DUNGEON_FATAL("dungeon triggter  blocks,coord = ",coord);
		return CbppResponse(Msg::ERR_BLOCKED_BY_TRIGGER_BLOCKS);
	}

	/*
	const unsigned border_thickness = Data::Global::as_unsigned(Data::Global::SLOT_MAP_BORDER_THICKNESS);
	if((map_x < border_thickness) || (map_y < border_thickness))
	{
		LOG_EMPERY_DUNGEON_TRACE("Blocked by map border: coord = ", coord);
		return CbppResponse(Msg::ERR_BLOCKED_BY_IMPASSABLE_MAP_CELL) <<coord;
	}
	*/
	std::vector<boost::shared_ptr<DungeonObject>> adjacent_objects;
	dungeon->get_dungeon_objects_by_rectangle(adjacent_objects,
		Rectangle(Coord(coord.x() - 3, coord.y() - 3), Coord(coord.x() + 4, coord.y() + 4)));
	std::vector<Coord> foundation;
	for(auto it = adjacent_objects.begin(); it != adjacent_objects.end(); ++it){
		const auto &other_object = *it;
		const auto other_dungeon_object_uuid = other_object->get_dungeon_object_uuid();
		const auto other_coord = other_object->get_coord();
		if(coord == other_coord){
			LOG_EMPERY_DUNGEON_TRACE("Blocked by another map object: other_map_object_uuid = ", other_dungeon_object_uuid);
			if(wait_for_moving_objects && other_object->is_moving()){
				return CbppResponse(Msg::ERR_BLOCKED_BY_TROOPS_TEMPORARILY) <<other_dungeon_object_uuid;
			}
			return CbppResponse(Msg::ERR_BLOCKED_BY_TROOPS) <<other_dungeon_object_uuid;
		}
	}

	return CbppResponse();
}

bool get_dungeon_coord_passable(DungeonTypeId dungeonTypeId, Coord coord){
	PROFILE_ME;

	const auto dungeon_data = Data::Dungeon::require(dungeonTypeId);
	const auto map_x = coord.x();
	const auto map_y = coord.y();
	const auto cell_data = Data::MapCellBasic::get(dungeon_data->dungeon_map,map_x, map_y);
	if(!cell_data){
		LOG_EMPERY_DUNGEON_FATAL("cell_data is null");
		return false;
	}
	const auto terrain_id = cell_data->terrain_id;
	const auto terrain_data = Data::MapTerrain::require(terrain_id);
	return terrain_data->passable;
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
	Coord from_coord, Coord to_coord, DungeonUuid dungeon_uuid, AccountUuid account_uuid, std::uint64_t distance_limit, std::uint64_t distance_close_enough)
{
	PROFILE_ME;
	LOG_EMPERY_DUNGEON_DEBUG("Pathfinding: from_coord = ", from_coord, ", to_coord = ", to_coord,
		", distance_limit = ", distance_limit, ", distance_close_enough = ", distance_close_enough);

	if(from_coord == to_coord){
		return true;
	}

	boost::container::flat_map<Coord, AStarCoordElement> astar_coords;
	astar_coords.reserve(distance_limit * distance_limit * 4);
	std::vector<AStarCoordElement> coords_open;
	coords_open.reserve(distance_limit * 2);

	AStarCoordElement init_elem = { };
	init_elem.coord            = from_coord;
	init_elem.closed           = false;
	init_elem.distance_from    = 0;
	init_elem.distance_to_hint = UINT64_MAX;
	astar_coords.emplace(from_coord, init_elem);
	coords_open.emplace_back(init_elem);

	std::uint64_t nearest_dist = UINT64_MAX;
	Coord nearest_coord;
	
	std::vector<Coord> surrounding;
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
				//除去地图中不存在的点
				const auto dungeon = DungeonMap::require(dungeon_uuid);
				const auto dungeon_data = Data::Dungeon::require(dungeon->get_dungeon_type_id());
				const auto map_x = coord.x();
				const auto map_y = coord.y();
				const auto cell_data = Data::MapCellBasic::get(dungeon_data->dungeon_map,map_x, map_y);
				if(!cell_data){
					continue;
				}
				auto cit = astar_coords.find(coord);
				if(cit == astar_coords.end()){
					AStarCoordElement elem = { };
					elem.coord            = coord;
					const auto result = get_move_result(dungeon_uuid,coord, account_uuid, true);
					elem.closed           = (result.first != Msg::ST_OK);
					elem.distance_from    = UINT64_MAX;
					elem.distance_to_hint = get_distance_of_coords(coord, to_coord);
					cit = astar_coords.emplace(coord, elem).first;
				}
				auto &new_elem = cit->second;
				if(new_elem.closed){
					continue;
				}

				const auto new_distance_from = elem_popped.distance_from + 1;
				if(new_elem.distance_from <= new_distance_from){
					continue;
				}
				new_elem.distance_from = new_distance_from;
				new_elem.parent_coord = elem_popped.coord;

				if(new_elem.distance_to_hint <= distance_close_enough){
					// 寻路成功。
					nearest_coord = coord;
					nearest_dist = 0;
					goto _done;
				}
				if(new_elem.distance_to_hint < nearest_dist){
					nearest_dist = new_elem.distance_to_hint;
					nearest_coord = coord;
				}
				if(new_distance_from < distance_limit){
					coords_open.emplace_back(new_elem);
					std::push_heap(coords_open.begin(), coords_open.end(), &compare_astar_coords_by_distance_hint);
				}
			}
		}

		if(coords_open.empty()){
		LOG_EMPERY_DUNGEON_DEBUG("Pathfinding failed: from_coord = ", from_coord, ", to_coord = ", to_coord,
		", distance_limit = ", distance_limit, ", distance_close_enough = ", distance_close_enough);
			goto _done;
		}
	}
	_done:
	;
	if(nearest_dist != UINT64_MAX){
		std::deque<Coord> coord_queue;
		auto current_coord = nearest_coord;
		for(;;){
			coord_queue.emplace_front(current_coord);
			if(current_coord == from_coord){
				break;
			}
			current_coord = astar_coords.at(current_coord).parent_coord;
		}
		assert(!coord_queue.empty());
		path.reserve(path.size() + coord_queue.size() - 1);
		for(auto qit = coord_queue.begin(), qprev = qit; ++qit != coord_queue.end(); qprev = qit){
			path.emplace_back(qit->x() - qprev->x(), qit->y() - qprev->y());
		}
	}
	return nearest_dist == 0;
}
}
