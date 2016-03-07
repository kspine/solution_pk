#include "precompiled.hpp"
#include "map_utilities.hpp"
#include "cbpp_response.hpp"
#include "data/global.hpp"
#include "data/map.hpp"
#include "map_object.hpp"
#include "map_cell.hpp"
#include "overlay.hpp"
#include "singletons/world_map.hpp"
#include "msg/err_map.hpp"
#include "map_object_type_ids.hpp"

namespace EmperyCenter {

std::uint64_t get_distance_of_coords(Coord lhs, Coord rhs){
	PROFILE_ME;

	// 求 rhs 所在点的 60°（斜率 2，截距 b1）和 120°（斜率 -2，截距 b2）的两条直线的截距。
	auto b1 = rhs.y() - rhs.x() * 2;
	auto b2 = rhs.y() + rhs.x() * 2;

	const int rhs_in_odd_row = !!(rhs.y() & 1);
	const int lhs_in_odd_row = !!(lhs.y() & 1);
	// 以偶数行为基准修正截距。
	b1 -= rhs_in_odd_row;
	b2 += rhs_in_odd_row;
	// 求上述两条直线和 lhs 所在横轴的交点横坐标。
	b1 += lhs_in_odd_row;
	b2 -= lhs_in_odd_row;
	const auto x1 = (lhs.y() - b1) / 2;
	const auto x2 = (b2 - lhs.y()) / 2;
	return static_cast<std::uint64_t>(std::max({
		std::abs(x1 - lhs.x()), std::abs(x2 - lhs.x()), std::abs(rhs.y() - lhs.y())
		}));
}

namespace {
	std::array<std::array<std::vector<Coord>, 2>, 64> g_surrounding_table;

	void generate_surrounding_coords(std::vector<Coord> &ret, Coord origin, std::uint64_t radius, bool in_odd_row){
		PROFILE_ME;
		LOG_EMPERY_CENTER_DEBUG("Generating surrounding coords: radius = ", radius);

		assert(radius >= 1);

		ret.reserve(ret.size() + radius * 6);
		auto current = Coord(origin.x() - static_cast<std::int64_t>(radius), origin.y());
		int dx = in_odd_row;
		for(std::uint64_t i = 0; i < radius; ++i){
			ret.emplace_back(current);
			current = Coord(current.x() + dx, current.y() + 1);
			dx ^= 1;
		}
		for(std::uint64_t i = 0; i < radius; ++i){
			ret.emplace_back(current);
			current = Coord(current.x() + 1, current.y());
		}
		for(std::uint64_t i = 0; i < radius; ++i){
			ret.emplace_back(current);
			current = Coord(current.x() + dx, current.y() - 1);
			dx ^= 1;
		}
		for(std::uint64_t i = 0; i < radius; ++i){
			ret.emplace_back(current);
			dx ^= 1;
			current = Coord(current.x() - dx, current.y() - 1);
		}
		for(std::uint64_t i = 0; i < radius; ++i){
			ret.emplace_back(current);
			current = Coord(current.x() - 1, current.y());
		}
		for(std::uint64_t i = 0; i < radius; ++i){
			ret.emplace_back(current);
			dx ^= 1;
			current = Coord(current.x() - dx, current.y() + 1);
		}
	}
}

void get_surrounding_coords(std::vector<Coord> &ret, Coord origin, std::uint64_t radius){
	PROFILE_ME;

	if(radius == 0){
		ret.emplace_back(origin);
		return;
	}

	const bool in_odd_row = origin.y() & 1;
	if(radius - 1 >= g_surrounding_table.size()){
		LOG_EMPERY_CENTER_DEBUG("Radius is too large to use a lookup table: radius = ", radius);
		generate_surrounding_coords(ret, origin, radius, in_odd_row);
	} else {
		auto &table = g_surrounding_table.at(radius - 1).at(in_odd_row);
		if(table.empty()){
			LOG_EMPERY_CENTER_DEBUG("Generating lookup table: radius = ", radius, ", in_odd_row = ", in_odd_row);
			std::vector<Coord> new_table;
			generate_surrounding_coords(new_table, Coord(0, 0), radius, in_odd_row);
			table.swap(new_table);
		} else {
			LOG_EMPERY_CENTER_TRACE("Using existent lookup table: radius = ", radius, ", in_odd_row = ", in_odd_row);
		}
		ret.reserve(ret.size() + table.size());
		for(auto it = table.begin(); it != table.end(); ++it){
			ret.emplace_back(origin.x() + it->x(), origin.y() + it->y());
		}
	}
}

void get_castle_foundation(std::vector<Coord> &ret, Coord origin, bool solid){
	PROFILE_ME;

	static constexpr auto castle_foundation_table =
		std::array<std::array<std::pair<std::int8_t, std::int8_t>, 10>, 2>{{
			{{
				{ 0, 0}, { 1, 0},
				{-1, 0}, {-1,-1}, { 0,-1}, { 1,-1}, { 2, 0}, { 1, 1}, { 0, 1}, {-1, 1}
			}}, {{
				{ 0, 0}, { 1, 0},
				{-1, 0}, { 0,-1}, { 1,-1}, { 2,-1}, { 2, 0}, { 2, 1}, { 1, 1}, { 0, 1}
			}},
		}};
	static constexpr std::ptrdiff_t outline_offset = 2;

	const bool in_odd_row = origin.y() & 1;
	const auto &table = castle_foundation_table.at(in_odd_row);
	auto begin = table.begin();
	if(!solid){
		begin += outline_offset;
	}
	ret.reserve(ret.size() + static_cast<std::size_t>(table.end() - begin));
	for(auto it = begin; it != table.end(); ++it){
		ret.emplace_back(origin.x() + it->first, origin.y() + it->second);
	}
}

std::pair<long, std::string> can_deploy_castle_at(Coord coord, MapObjectUuid excluding_map_object_uuid){
	PROFILE_ME;

	using Response = CbppResponse;

	std::vector<boost::shared_ptr<MapObject>> other_map_objects;

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
		const unsigned border_thickness = Data::Global::as_unsigned(Data::Global::SLOT_MAP_BORDER_THICKNESS);
		if((map_x < border_thickness) || (map_x >= cluster_scope.width() - border_thickness) ||
			(map_y < border_thickness) || (map_y >= cluster_scope.height() - border_thickness))
		{
			return Response(Msg::ERR_CANNOT_DEPLOY_IMMIGRANTS_HERE) <<foundation_coord;
		}

		std::vector<boost::shared_ptr<MapCell>> map_cells;
		WorldMap::get_map_cells_by_rectangle(map_cells, Rectangle(foundation_coord, 1, 1));
		for(auto it = map_cells.begin(); it != map_cells.end(); ++it){
			const auto &map_cell = *it;
			if(!map_cell->is_virtually_removed()){
				return Response(Msg::ERR_CANNOT_DEPLOY_ON_TERRITORY) <<foundation_coord;
			}
		}

		std::vector<boost::shared_ptr<Overlay>> overlays;
		WorldMap::get_overlays_by_rectangle(overlays, Rectangle(foundation_coord, 1, 1));
		for(auto it = overlays.begin(); it != overlays.end(); ++it){
			const auto &overlay = *it;
			if(!overlay->is_virtually_removed()){
				return Response(Msg::ERR_CANNOT_DEPLOY_ON_OVERLAY) <<foundation_coord;
			}
		}

		other_map_objects.clear();
		WorldMap::get_map_objects_by_rectangle(other_map_objects, Rectangle(foundation_coord, 1, 1));
		for(auto oit = other_map_objects.begin(); oit != other_map_objects.end(); ++oit){
			const auto &other_object = *oit;
			if(other_object->get_map_object_uuid() == excluding_map_object_uuid){
				continue;
			}
			if(!other_object->is_virtually_removed()){
				return Response(Msg::ERR_CANNOT_DEPLOY_ON_TROOPS) <<foundation_coord;
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
	other_map_objects.clear();
	WorldMap::get_map_objects_by_rectangle(other_map_objects, Rectangle(Coord(other_left, other_bottom), Coord(other_right, other_top)));
	for(auto it = other_map_objects.begin(); it != other_map_objects.end(); ++it){
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
