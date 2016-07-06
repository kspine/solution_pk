#include "../precompiled.hpp"
#include "common.hpp"
#include "../../../empery_center/src/msg/st_friend.hpp"
#include "../../../empery_center/src/msg/ts_friend.hpp"
#include "../../../empery_center/src/msg/err_friend.hpp"

namespace EmperyController {

CONTROLLER_SERVLET(Msg::ST_FriendCompareExchangeSync, controller, req){
	const auto account_uuid = AccountUuid(req.account_uuid);
	const auto account = AccountMap::get(account_uuid);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_ACCOUNT) <<account_uuid;
	}
	const auto friend_uuid = AccountUuid(req.friend_uuid);
	const auto friend_account = AccountMap::get(friend_uuid);
	if(!friend_account){
		return Response(Msg::ERR_NO_SUCH_ACCOUNT) <<friend_uuid;
	}

	const auto using_controller = account->try_set_controller(controller);

	Msg::TS_FriendCompareExchange msg;
	msg.account_uuid      = account_uuid.str();
	msg.category_expected = req.category_expected;
	msg.category          = req.category;
	msg.max_count         = req.max_count;
	msg.friend_uuid       = friend_uuid.str();
	msg.metadata          = std::move(req.metadata);
	auto result = using_controller->send_and_wait(msg);
	if(result.first != Msg::ST_OK){
		return std::move(result);
	}

	return Response();
}

CONTROLLER_SERVLET(Msg::ST_FriendRemoveSync, controller, req){
	const auto account_uuid = AccountUuid(req.account_uuid);
	const auto account = AccountMap::get(account_uuid);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_ACCOUNT) <<account_uuid;
	}
	const auto friend_uuid = AccountUuid(req.friend_uuid);
	const auto friend_account = AccountMap::get(friend_uuid);
	if(!friend_account){
		return Response(Msg::ERR_NO_SUCH_ACCOUNT) <<friend_uuid;
	}

	const auto using_controller = account->try_set_controller(controller);

	Msg::TS_FriendRemove msg;
	msg.account_uuid = account_uuid.str();
	msg.friend_uuid  = friend_uuid.str();
	auto result = using_controller->send_and_wait(msg);
	if(result.first != Msg::ST_OK){
		return std::move(result);
	}

	return Response();
}

}
