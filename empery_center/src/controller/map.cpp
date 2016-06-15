#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/err_map.hpp"
#include "../msg/err_castle.hpp"
#include "../msg/ts_map.hpp"
#include "../singletons/world_map.hpp"
#include "../castle.hpp"

namespace EmperyCenter {

CONTROLLER_SERVLET(Msg::TS_MapInvalidateCastle, controller, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = WorldMap::forced_reload_castle(map_object_uuid);
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}

	WorldMap::forced_reload_child_map_objects(map_object_uuid);

	return Response();
}

}
