#ifndef EMPERY_CENTER_SINGLETONS_MAP_EVENT_BLOCK_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_MAP_EVENT_BLOCK_MAP_HPP_

#include "../id_types.hpp"
#include "../coord.hpp"
#include <boost/shared_ptr.hpp>
#include <vector>

namespace EmperyCenter {

class MapEventBlock;

struct MapEventBlockMap {
	static boost::shared_ptr<MapEventBlock> get(Coord coord);
	static boost::shared_ptr<MapEventBlock> require(Coord coord);
	static void insert(const boost::shared_ptr<MapEventBlock> &map_event_block);
	static void update(const boost::shared_ptr<MapEventBlock> &map_event_block, bool throws_if_not_exists = true);

	static void get_all(std::vector<boost::shared_ptr<MapEventBlock>> &ret);

	static Coord get_block_coord_from_world_coord(Coord coord);

private:
	MapEventBlockMap() = delete;
};

}

#endif
