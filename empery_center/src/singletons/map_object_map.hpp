#ifndef EMPERY_CENTER_SINGLETONS_MAP_OBJECT_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_MAP_OBJECT_MAP_HPP_

#include "../id_types.hpp"
#include "../coord.hpp"
#include "../rectangle.hpp"
#include <boost/shared_ptr.hpp>
#include <vector>

namespace EmperyCenter {

class MapObject;

struct MapObjectMap {
	static boost::shared_ptr<MapObject> get(const MapObjectUuid &mapObjectUuid);
	static void update(const boost::shared_ptr<MapObject> &mapObject, const Coord &coord);
	static void remove(const MapObjectUuid &mapObjectUuid) noexcept;

	static void getByRectangle(std::vector<std::pair<Coord, boost::shared_ptr<MapObject>>> &ret, const Rectangle &rectangle);

private:
	MapObjectMap();
};

}

#endif
