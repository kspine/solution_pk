#ifndef EMPERY_CENTER_MAP_UTILITIES_CENTER_HPP_
#define EMPERY_CENTER_MAP_UTILITIES_CENTER_HPP_

#include <utility>
#include <string>
#include <boost/shared_ptr.hpp>
#include "id_types.hpp"
#include "coord.hpp"

namespace EmperyCenter {

class Castle;
class MapObject;
class MapCell;

extern std::pair<long, std::string> can_place_defense_building_at(Coord coord);
extern std::pair<long, std::string> can_deploy_castle_at(Coord coord, MapObjectUuid excluding_map_object_uuid, bool force_placing);

extern void create_resource_crates(Coord origin, ResourceId resource_id, std::uint64_t amount,
	unsigned radius_inner, unsigned radius_outer, MapObjectTypeId map_object_type_id);

extern void async_hang_up_castle(const boost::shared_ptr<Castle> &castle) noexcept;

extern std::pair<long, std::string> is_under_protection(const boost::shared_ptr<MapObject> &attacking_object,
	const boost::shared_ptr<MapObject> &attacked_object);
extern std::pair<long, std::string> is_under_protection(const boost::shared_ptr<MapObject> &attacking_object,
	const boost::shared_ptr<MapCell> &attacked_cell);

}

#endif
