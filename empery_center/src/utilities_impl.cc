#ifndef EMPERY_CENTER_UTILITIES_NAMESPACE_
#	error Do not use this file directly!
#endif

#include "log.hpp"

namespace EMPERY_CENTER_UTILITIES_NAMESPACE_ {

boost::uint64_t get_distance_of_coords(Coord lhs, Coord rhs){
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
	return static_cast<boost::uint64_t>(std::max({
		std::abs(x1 - lhs.x()), std::abs(x2 - lhs.x()), std::abs(rhs.y() - lhs.y())
		}));
}

namespace {
	boost::array<boost::array<std::vector<Coord>, 2>, 64> g_surrounding_table;

	void generate_surrounding_coords(std::vector<Coord> &ret, Coord origin, boost::uint64_t radius, bool in_odd_row){
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

void get_surrounding_coords(std::vector<Coord> &ret, Coord origin, boost::uint64_t radius){
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

void get_castle_foundation(std::vector<Coord> &ret, Coord origin){
	PROFILE_ME;

	static constexpr boost::array<boost::array<Coord, 8>, 2> castle_foundation_table = {{
		{{ {-1, 0}, {-1, -1}, {0, -1}, {1, -1}, {2, 0}, {1, 1}, {0, 1}, {-1, 1} }},
		{{ {-1, 0}, { 0, -1}, {1, -1}, {2, -1}, {2, 0}, {2, 1}, {1, 1}, { 0, 1} }},
	}};

	const bool in_odd_row = origin.y() & 1;
	const auto &table = castle_foundation_table.at(in_odd_row);
	ret.reserve(ret.size() + table.size());
	for(auto it = table.begin(); it != table.end(); ++it){
		ret.emplace_back(origin.x() + it->x(), origin.y() + it->y());
	}
}

}

#undef EMPERY_CENTER_UTILITIES_NAMESPACE_
