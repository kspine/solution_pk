#ifndef EMPERY_CENTER_CASTLE_UTILITIES_HPP_
#define EMPERY_CENTER_CASTLE_UTILITIES_HPP_

#include <utility>
#include <string>
#include <boost/shared_ptr.hpp>
#include "id_types.hpp"
#include "coord.hpp"

namespace EmperyCenter {

extern std::pair<long, std::string> can_deploy_castle_at(Coord coord, MapObjectUuid excluding_map_object_uuid);

}

#endif
