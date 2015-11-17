#include "precompiled.hpp"
#include "utilities.hpp"

namespace EmperyCenter {

namespace {
	boost::array<boost::array<std::vector<Coord>, 2>, 64> g_surrounding_table;

	void generate_surrounding_coords(std::vector<Coord> &ret, const Coord &origin, boost::uint64_t radius, bool in_odd_row){
		PROFILE_ME;
		LOG_EMPERY_CENTER_DEBUG("Generating surrounding coords: radius = ", radius);

		assert(radius >= 1);

		ret.reserve(ret.size() + radius * 6);
		auto current = Coord(origin.x() - static_cast<boost::int64_t>(radius), origin.y());
		int dx = in_odd_row;
		for(boost::uint64_t i = 0; i < radius; ++i){
			ret.emplace_back(current);
			current = Coord(current.x() + dx, current.y() + 1);
			dx ^= 1;
		}
		for(boost::uint64_t i = 0; i < radius; ++i){
			ret.emplace_back(current);
			current = Coord(current.x() + 1, current.y());
		}
		for(boost::uint64_t i = 0; i < radius; ++i){
			ret.emplace_back(current);
			current = Coord(current.x() + dx, current.y() - 1);
			dx ^= 1;
		}
		for(boost::uint64_t i = 0; i < radius; ++i){
			ret.emplace_back(current);
			dx ^= 1;
			current = Coord(current.x() - dx, current.y() - 1);
		}
		for(boost::uint64_t i = 0; i < radius; ++i){
			ret.emplace_back(current);
			current = Coord(current.x() - 1, current.y());
		}
		for(boost::uint64_t i = 0; i < radius; ++i){
			ret.emplace_back(current);
			dx ^= 1;
			current = Coord(current.x() - dx, current.y() + 1);
		}
	}
}

void get_surrounding_coords(std::vector<Coord> &ret, const Coord &origin, boost::uint64_t radius){
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
			LOG_EMPERY_CENTER_DEBUG("Using existent lookup table: radius = ", radius, ", in_odd_row = ", in_odd_row);
		}
		ret.reserve(ret.size() + table.size());
		for(auto it = table.begin(); it != table.end(); ++it){
			ret.emplace_back(origin.x() + it->x(), origin.y() + it->y());
		}
	}
}

}
