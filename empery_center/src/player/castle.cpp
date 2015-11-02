#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/cs_castle.hpp"
#include "../msg/sc_castle.hpp"
#include "../msg/cerr_castle.hpp"

namespace EmperyCenter {

PLAYER_SERVLET(Msg::CS_CastleQueryInfo, accountUuid, session, req){
	return { };
}

PLAYER_SERVLET(Msg::CS_CastleCreateBuilding, accountUuid, session, req){
	return { };
}

PLAYER_SERVLET(Msg::CS_CastleCancelMission, accountUuid, session, req){
	return { };
}

PLAYER_SERVLET(Msg::CS_CastleUpgradeBuilding, accountUuid, session, req){
	return { };
}

PLAYER_SERVLET(Msg::CS_CastleDestroyBuilding, accountUuid, session, req){
	return { };
}

}
