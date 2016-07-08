#include "../precompiled.hpp"
#include "common.hpp"
#include "../../../empery_center/src/msg/st_friend.hpp"
#include "../../../empery_center/src/msg/ts_friend.hpp"
#include "../../../empery_center/src/msg/err_friend.hpp"
#include <poseidon/async_job.hpp>

namespace EmperyController {

CONTROLLER_SERVLET(Msg::ST_FriendPeerCompareExchange, controller, req){
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

	const auto using_controller = friend_account->try_set_controller(controller);
	const auto transaction_uuid = Poseidon::Uuid(req.transaction_uuid);

	Poseidon::enqueue_async_job(
		[=]{
			PROFILE_ME;
			try {
				Msg::TS_FriendPeerCompareExchange sreq;
				sreq.account_uuid      = account_uuid.str();
				sreq.friend_uuid       = friend_uuid.str();
				sreq.categories_expected.reserve(req.categories_expected.size());
				for(auto it = req.categories_expected.begin(); it != req.categories_expected.end(); ++it){
					auto &elem = *sreq.categories_expected.emplace(sreq.categories_expected.end());
					elem.category_expected = it->category_expected;
				}
				sreq.category          = req.category;
				sreq.max_count         = req.max_count;
				sreq.metadata          = std::move(req.metadata);
				auto result = using_controller->send_and_wait(sreq);

				Msg::TS_FriendPeerCompareExchangeResult sres;
				sres.account_uuid      = account_uuid.str();
				sres.transaction_uuid  = transaction_uuid.to_string();
				sres.err_code          = result.first;
				sres.err_msg           = std::move(result.second);
				controller->send(sres);
			} catch(std::exception &e){
				LOG_EMPERY_CONTROLLER_WARNING("std::exception thrown: what = ", e.what());
				controller->shutdown(e.what());
			}
		});

	return Response();
}

}
