#ifndef EMPERY_CENTER_SINGLETONS_DUNGEON_BOX_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_DUNGEON_BOX_MAP_HPP_

#include <boost/shared_ptr.hpp>
#include "../id_types.hpp"

namespace EmperyCenter {

class DungeonBox;

struct DungeonBoxMap {
	static boost::shared_ptr<DungeonBox> get(AccountUuid account_uuid);
	static boost::shared_ptr<DungeonBox> require(AccountUuid account_uuid);
	static void unload(AccountUuid account_uuid);

private:
	DungeonBoxMap() = delete;
};

}

#endif
