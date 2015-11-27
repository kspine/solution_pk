#ifndef EMPERY_CENTER_UTILITIES_NAMESPACE_
#	error Do not use this file directly!
#endif

#include <boost/cstdint.hpp>
#include <algorithm>
#include <vector>
#include "coord.hpp"
#include "log.hpp"

namespace EMPERY_CENTER_UTILITIES_NAMESPACE_ {

extern boost::uint64_t get_distance_of_coords(Coord lhs, Coord rhs);
extern void get_surrounding_coords(std::vector<Coord> &ret, Coord origin, boost::uint64_t radius);

}

#undef EMPERY_CENTER_UTILITIES_NAMESPACE_
