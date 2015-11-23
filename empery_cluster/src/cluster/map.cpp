#include "../precompiled.hpp"
#include "common.hpp"
#include "../../../empery_center/src/msg/ck_map.hpp"

namespace EmperyCluster {

CLUSTER_SERVLET(Msg::CK_MapClusterRegistrationSucceeded, client, msg){
	return Response();
}

CLUSTER_SERVLET(Msg::CK_MapAddMapObject, client, msg){
	return Response();
}

CLUSTER_SERVLET(Msg::CK_MapRemoveMapObject, client, msg){
	return Response();
}

CLUSTER_SERVLET(Msg::CK_MapSetWaypoints, client, msg){
	return Response();
}

}
