#ifndef EMPERY_CLUSTER_UTILITIES_HPP_
#define EMPERY_CLUSTER_UTILITIES_HPP_

#include "../../empery_center/src/map_utilities.hpp"

#include <vector>
#include <utility>
#include "id_types.hpp"
#include "coord.hpp"

namespace EmperyCluster {

using ::EmperyCenter::get_distance_of_coords;
using ::EmperyCenter::get_surrounding_coords;
using ::EmperyCenter::get_castle_foundation;

extern std::pair<long, std::string> get_move_result(Coord coord, AccountUuid account_uuid, bool wait_for_moving_objects);

extern bool find_path(std::vector<std::pair<signed char, signed char>> &path,
	Coord from_coord, Coord to_coord, AccountUuid account_uuid, std::uint64_t radius_limit, std::uint64_t distance_close_enough);

}

#endif
