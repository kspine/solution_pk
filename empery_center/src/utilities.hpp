#ifndef EMPERY_CENTER_UTILITIES_HPP_
#define EMPERY_CENTER_UTILITIES_HPP_

#include "coord.hpp"

#define EMPERY_CENTER_UTILITIES_NAMESPACE_  EmperyCenter
#include "utilities_decl.hh"

#include <utility>
#include <string>
#include <boost/shared_ptr.hpp>

namespace EmperyCenter {

class MapObject;

extern std::pair<long, std::string> can_deploy_castle_at(Coord coord, const boost::shared_ptr<MapObject> &excluding_map_object);

}

#endif
