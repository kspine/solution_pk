#ifndef EMPERY_CENTER_MAP_UTILITIES_HPP_
#define EMPERY_CENTER_MAP_UTILITIES_HPP_

#include <cstdint>
#include <vector>
#include "coord.hpp"
#include "id_types.hpp"

namespace EmperyCenter {

extern std::uint64_t get_distance_of_coords(Coord lhs, Coord rhs);
extern void get_surrounding_coords(std::vector<Coord> &ret, Coord origin, std::uint64_t radius);

extern std::size_t get_castle_foundation_solid_area();
extern void get_castle_foundation(std::vector<Coord> &ret, Coord origin, bool solid);

}

#endif
