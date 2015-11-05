#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/ks_map.hpp"
#include "../msg/kerr_map.hpp"
#include "../singletons/cluster_session_map.hpp"

namespace EmperyCenter {

CLUSTER_SERVLET(Msg::KS_MapRegisterCluster, session, req){
	const auto oldSession = ClusterSessionMap::get(req.mapX, req.mapY);
	if(oldSession){
		LOG_EMPERY_CENTER_WARNING("Cluster server conflict: mapX = ", req.mapX, ", mapY = ", req.mapY);
		return CbppResponse(Msg::KERR_MAP_SERVER_CONFLICT) <<"Cluster server conflict";
	}
	ClusterSessionMap::set(req.mapX, req.mapY, session);
	return CbppResponse();
}

}
