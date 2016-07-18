#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/ds_dungeon.hpp"
#include "../msg/err_dungeon.hpp"
#include "../singletons/dungeon_map.hpp"

namespace EmperyCenter {

DUNGEON_SERVLET(Msg::DS_DungeonRegisterServer, server, /* req */){
	DungeonMap::add_server(server);

	return Response();
}

}
