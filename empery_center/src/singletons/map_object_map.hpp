#ifndef EMPERY_CENTER_SINGLETONS_MAP_OBJECT_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_MAP_OBJECT_MAP_HPP_

#include "../id_types.hpp"
#include "../coord.hpp"
#include "../rectangle.hpp"
#include <boost/shared_ptr.hpp>
#include <vector>

namespace EmperyCenter {

class MapObject;
class PlayerSession;

struct MapObjectMap {
	static boost::shared_ptr<MapObject> get(MapObjectUuid map_object_uuid);
	static void update(const boost::shared_ptr<MapObject> &map_object);
	static void remove(MapObjectUuid map_object_uuid) noexcept;

	static void get_by_owner(std::vector<boost::shared_ptr<MapObject>> &ret, AccountUuid owner_uuid);

	static void get_by_rectangle(std::vector<std::pair<Coord, boost::shared_ptr<MapObject>>> &ret, const Rectangle &rectangle);

	static void set_player_view(const boost::shared_ptr<PlayerSession> &session, const Rectangle &view);
	static void synchronize_player_view(const boost::shared_ptr<PlayerSession> &session, const Rectangle &view) noexcept;

private:
	MapObjectMap() = delete;
};

}

#endif
