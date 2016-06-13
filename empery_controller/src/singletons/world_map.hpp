#ifndef EMPERY_CONTROLLER_SINGLETONS_WORLD_MAP_HPP_
#define EMPERY_CONTROLLER_SINGLETONS_WORLD_MAP_HPP_

#include "../id_types.hpp"
#include "../coord.hpp"
#include <boost/shared_ptr.hpp>
#include <vector>

namespace EmperyController {

class Castle;

struct WorldMap {
	static boost::shared_ptr<Castle> get_castle(MapObjectUuid map_object_uuid);
	static boost::shared_ptr<Castle> require_castle(MapObjectUuid map_object_uuid);
	static boost::shared_ptr<Castle> get_or_reload_castle(MapObjectUuid map_object_uuid);
	static boost::shared_ptr<Castle> forced_reload_castle(MapObjectUuid map_object_uuid);

	static void get_castles_by_owner(std::vector<boost::shared_ptr<Castle>> &ret, AccountUuid owner_uuid);
	static void get_castles_by_parent_object(std::vector<boost::shared_ptr<Castle>> &ret, MapObjectUuid parent_object_uuid);

private:
	WorldMap() = delete;
};

}

#endif
