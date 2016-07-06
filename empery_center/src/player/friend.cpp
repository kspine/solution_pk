#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/cs_friend.hpp"
#include "../msg/sc_friend.hpp"
#include "../msg/err_friend.hpp"
#include "../singletons/friend_box_map.hpp"
#include "../friend_box.hpp"

namespace EmperyCenter {

PLAYER_SERVLET(Msg::CS_FriendGetAll, account, session, /* req */){
	const auto friend_box = FriendBoxMap::require(account->get_account_uuid());
	friend_box->pump_status();

	friend_box->synchronize_with_player(session);

	return Response();
}

PLAYER_SERVLET(Msg::CS_FriendRequest, account, session, /* req */){

	return Response();
}

PLAYER_SERVLET(Msg::CS_FriendAccept, account, session, /* req */){

	return Response();
}

PLAYER_SERVLET(Msg::CS_FriendDecline, account, session, /* req */){

	return Response();
}

PLAYER_SERVLET(Msg::CS_FriendDelete, account, session, /* req */){

	return Response();
}

}
