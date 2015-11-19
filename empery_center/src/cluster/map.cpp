#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/ks_map.hpp"
#include "../msg/kerr_map.hpp"
#include "../singletons/cluster_session_map.hpp"

namespace EmperyCenter {

CLUSTER_SERVLET(Msg::KS_MapRegisterCluster, session, req){
	const auto server_coord = Coord(req.server_x, req.server_y);
	const auto old_session = ClusterSessionMap::get(server_coord);
	if(old_session){
		LOG_EMPERY_CENTER_WARNING("Cluster server conflict: server_coord = ", server_coord);
		return Response(Msg::KERR_MAP_SERVER_CONFLICT) <<"Cluster server conflict";
	}

	ClusterSessionMap::set(server_coord, session);

	return Response();
}

}
