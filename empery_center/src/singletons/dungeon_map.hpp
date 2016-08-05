#ifndef EMPERY_CENTER_SINGLETONS_DUNGEON_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_DUNGEON_MAP_HPP_

#include <boost/shared_ptr.hpp>
#include <vector>
#include "../id_types.hpp"

namespace EmperyCenter {

class Dungeon;
class DungeonSession;

struct DungeonMap {
	static boost::shared_ptr<Dungeon> get(DungeonUuid dungeon_uuid);
	static boost::shared_ptr<Dungeon> require(DungeonUuid dungeon_uuid);
	static void get_by_founder(std::vector<boost::shared_ptr<Dungeon>> &ret, AccountUuid founder_uuid);

	static void insert(const boost::shared_ptr<Dungeon> &dungeon);
	static void update(const boost::shared_ptr<Dungeon> &dungeon, bool throws_if_not_exists = true);

	static boost::shared_ptr<DungeonSession> pick_server();
	static void add_server(const boost::shared_ptr<DungeonSession> &server);

private:
	DungeonMap() = delete;
};

}

#endif
