#include "../precompiled.hpp"
#include "common.hpp"

namespace EmperyCenter {

AccountUuid requireAccountUuidBySession(const boost::shared_ptr<PlayerSession> &session){
	PROFILE_ME;

	auto accountUuid = PlayerSessionMap::getAccountUuid(session);
	if(!accountUuid){
		PLAYER_THROW(Msg::ERR_NOT_LOGGED_IN);
	}
	return accountUuid;
}

}
