#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/err_map.hpp"
#include "../msg/err_castle.hpp"
#include "../msg/ts_map.hpp"
#include "../singletons/world_map.hpp"
#include "../castle.hpp"
#include <poseidon/singletons/mysql_daemon.hpp>

namespace EmperyCenter {

CONTROLLER_SERVLET(Msg::TS_MapInvalidateCastle, controller, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto coord = Coord(req.coord_x, req.coord_y);
	const auto cluster = WorldMap::get_cluster(coord);
	if(!cluster){
		LOG_EMPERY_CENTER_WARNING("Moving castle to non-existent cluster? coord = ", coord);
		return Response(Msg::ERR_CLUSTER_CONNECTION_LOST) <<coord;
	}
	Poseidon::MySqlDaemon::enqueue_for_waiting_for_all_async_operations();
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::forced_reload_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	const auto new_coord = castle->get_coord();
	if(new_coord != coord){
		return Response(Msg::ERR_BROKEN_PATH) <<new_coord;
	}

	WorldMap::forced_reload_map_objects_by_parent_object(map_object_uuid);

	return Response();
}

CONTROLLER_SERVLET(Msg::TS_MapRemoveMapObject, controller, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto map_object = WorldMap::get_map_object(map_object_uuid);
	if(!map_object){
		return Response(Msg::ERR_NO_SUCH_MAP_OBJECT) <<map_object_uuid;
	}

	LOG_EMPERY_CENTER_DEBUG("Deleting map object: map_object_uuid = ", map_object_uuid);
	map_object->delete_from_game();
	Poseidon::MySqlDaemon::enqueue_for_waiting_for_all_async_operations();

	return Response();
}

}
