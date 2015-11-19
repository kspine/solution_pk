#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/kc_map.hpp"
#include "../msg/err_map.hpp"
#include "../msg/kill.hpp"
#include "../singletons/cluster_session_map.hpp"

namespace EmperyCenter {

CLUSTER_SERVLET(Msg::KC_MapRegisterCluster, session, req){
	const auto server_coord = Coord(req.server_x, req.server_y);
	const auto old_session = ClusterSessionMap::get(server_coord);
	if(old_session){
		LOG_EMPERY_CENTER_ERROR("Cluster server conflict: server_coord = ", server_coord);
		return Response(Msg::KILL_MAP_SERVER_CONFLICT) <<server_coord;
	}

	ClusterSessionMap::set(server_coord, session);

	return Response();
}

}
