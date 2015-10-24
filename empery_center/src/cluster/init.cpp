#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/kc_init.hpp"
#include "../msg/kerr_init.hpp"
#include "../singletons/cluster_session_map.hpp"

namespace EmperyCenter {

CLUSTER_SERVLET(Msg::KC_InitRegisterCluster, session, req){
	const auto oldSession = ClusterSessionMap::get(req.mapX, req.mapY);
	if(oldSession){
		LOG_EMPERY_CENTER_WARNING("Cluster server conflict: mapX = ", req.mapX, ", mapY = ", req.mapY);
		return { Msg::ERR_MAP_SERVER_CONFLICT, { } };
	}
	ClusterSessionMap::set(req.mapX, req.mapY, session);
	return { };
}

}
