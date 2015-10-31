#include "precompiled.hpp"
#include "utilities.hpp"

namespace EmperyCenter {

namespace {
	boost::array<boost::array<std::vector<Coord>, 2>, 64> g_surroundingTable;

	void generateSurroundingCoords(std::vector<Coord> &ret, const Coord &origin, boost::uint64_t radius, bool oddRow){
		PROFILE_ME;
		LOG_EMPERY_CENTER_DEBUG("Generating surrounding coords: radius = ", radius);

		assert(radius >= 1);

		ret.reserve(ret.size() + radius * 6);
		auto current = Coord(origin.x() - static_cast<boost::int64_t>(radius), origin.y());
		int dx = oddRow;
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

void getSurroundingCoords(std::vector<Coord> &ret, const Coord &origin, boost::uint64_t radius){
	PROFILE_ME;

	if(radius == 0){
		ret.emplace_back(origin);
		return;
	}

	const bool oddRow = origin.y() & 1;
	if(radius - 1 >= g_surroundingTable.size()){
		LOG_EMPERY_CENTER_DEBUG("Radius is too large to use a lookup table: radius = ", radius);
		generateSurroundingCoords(ret, origin, radius, oddRow);
	} else {
		auto &table = g_surroundingTable.at(radius - 1).at(oddRow);
		if(table.empty()){
			LOG_EMPERY_CENTER_DEBUG("Generating lookup table: radius = ", radius);
			std::vector<Coord> newTable;
			generateSurroundingCoords(newTable, Coord(0, 0), radius, oddRow);
			table.swap(newTable);
		} else {
			LOG_EMPERY_CENTER_DEBUG("Using existent lookup table: radius = ", radius);
		}
		ret.reserve(ret.size() + table.size());
		for(auto it = table.begin(); it != table.end(); ++it){
			ret.emplace_back(origin.x() + it->x(), origin.y() + it->y());
		}
	}
}

}
