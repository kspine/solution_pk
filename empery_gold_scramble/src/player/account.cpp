#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/cs_account.hpp"
#include "../msg/sc_account.hpp"
#include "../msg/err_account.hpp"
#include "../singletons/promotion_daemon.hpp"

namespace EmperyGoldScramble {

PLAYER_SERVLET_RAW(Msg::CS_AccountLogin, session, req){
	const auto loginName = PromotionDaemon::getLoginNameFromSessionId(req.sessionId);
	if(loginName.empty()){
		return Response(Msg::ERR_INVALID_SESSION_ID) <<req.sessionId;
	}

	PlayerSessionMap::add(loginName, session);
	LOG_EMPERY_GOLD_SCRAMBLE_INFO("Account logged in: sessionId = ", req.sessionId, ", loginName = ", loginName);

	return Response();
}

PLAYER_SERVLET(Msg::CS_AccountBidUsingGoldCoins, loginName, session, req){
	return Response();
}

PLAYER_SERVLET(Msg::CS_AccountBidUsingAccountBalance, loginName, session, req){
	return Response();
}

}
