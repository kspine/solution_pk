#ifndef EMPERY_CONTROLLER_SINGLETONS_WORLD_MAP_HPP_
#define EMPERY_CONTROLLER_SINGLETONS_WORLD_MAP_HPP_

#include "../id_types.hpp"
#include "../coord.hpp"
#include "../rectangle.hpp"
#include <boost/shared_ptr.hpp>
#include <vector>

namespace EmperyController {

class Castle;
class ControllerSession;

struct WorldMap {
	static boost::shared_ptr<Castle> get_castle(MapObjectUuid map_object_uuid);
	static boost::shared_ptr<Castle> require_castle(MapObjectUuid map_object_uuid);
	static boost::shared_ptr<Castle> get_or_reload_castle(MapObjectUuid map_object_uuid);
	static boost::shared_ptr<Castle> forced_reload_castle(MapObjectUuid map_object_uuid);

	static void get_castles_by_owner(std::vector<boost::shared_ptr<Castle>> &ret, AccountUuid owner_uuid);
	static void get_castles_by_parent_object(std::vector<boost::shared_ptr<Castle>> &ret, MapObjectUuid parent_object_uuid);

	static Rectangle get_cluster_scope(Coord coord);

	static boost::shared_ptr<ControllerSession> get_controller(Coord coord);
	static void get_all_controllers(std::vector<std::pair<Coord, boost::shared_ptr<ControllerSession>>> &ret);
	static void set_controller(const boost::shared_ptr<ControllerSession> &controller, Coord coord);

private:
	WorldMap() = delete;
};

}

#endif
