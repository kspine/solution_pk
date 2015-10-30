#ifndef EMPERY_CENTER_UTILITIES_HPP_
#define EMPERY_CENTER_UTILITIES_HPP_

#include <boost/cstdint.hpp>
#include <vector>
#include "coord.hpp"

namespace EmperyCenter {

inline bool areCoordsAdjacent(const Coord &lhs, const Coord &rhs){
	if(lhs.y() == rhs.y()){
		return (lhs.x() == rhs.x() - 1) || (lhs.x() == rhs.x() + 1);
	}
	if((lhs.y() == rhs.y() - 1) || (lhs.y() == rhs.y() + 1)){
		return (lhs.x() == rhs.x()) || ((lhs.y() & 1) ? (lhs.x() == rhs.x() - 1) : (lhs.x() == rhs.x() + 1));
	}
	return false;
}

extern void getSurroundingCoords(std::vector<Coord> &ret, const Coord &origin, boost::uint64_t radius);

}

#endif
