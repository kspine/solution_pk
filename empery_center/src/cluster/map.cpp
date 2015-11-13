#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/ks_map.hpp"
#include "../msg/kerr_map.hpp"
#include "../singletons/cluster_session_map.hpp"

namespace EmperyCenter {

CLUSTER_SERVLET(Msg::KS_MapRegisterCluster, session, req){
	const auto old_session = ClusterSessionMap::get(req.map_x, req.map_y);
	if(old_session){
		LOG_EMPERY_CENTER_WARNING("Cluster server conflict: map_x = ", req.map_x, ", map_y = ", req.map_y);
		return Response(Msg::KERR_MAP_SERVER_CONFLICT) <<"Cluster server conflict";
	}
	ClusterSessionMap::set(req.map_x, req.map_y, session);
	return Response();
}

}
