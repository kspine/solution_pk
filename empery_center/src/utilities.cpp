#include "precompiled.hpp"
#include "utilities.hpp"

namespace EmperyCenter {

namespace {
	std::vector<std::vector<Coord>> g_surroundingTable;
}

void getSurroundingCoords(std::vector<Coord> &ret, const Coord &origin, boost::uint64_t radius){
	PROFILE_ME;

	if(radius == 0){
		ret.emplace_back(origin);
		return;
	}

	ret.reserve(ret.size() + radius * 6);
}

}
