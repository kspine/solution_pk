#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/err_friend.hpp"
#include "../msg/ts_friend.hpp"
#include "../singletons/friend_box_map.hpp"
#include "../friend_box.hpp"

namespace EmperyCenter {

CONTROLLER_SERVLET(Msg::TS_FriendPeerCompareExchange, controller, req){
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

	const auto friend_box = FriendBoxMap::get(friend_uuid);
	if(!friend_box){
		return Response(Msg::ERR_CONTROLLER_TOKEN_NOT_ACQUIRED) <<friend_uuid;
	}

	auto info = friend_box->get(account_uuid);
	if(std::find_if(req.categories_expected.begin(), req.categories_expected.end(),
		[&](decltype(req.categories_expected.front()) &elem){
			return info.category == static_cast<FriendBox::Category>(elem.category_expected);
		}) == req.categories_expected.end())
	{
		LOG_EMPERY_CENTER_DEBUG("Friend compare-and-exchange failure: account_uuid = ", account_uuid, ", friend_uuid = ", friend_uuid,
			", req = ", req, ", category = ", (int)info.category);
		return Response(Msg::ERR_FRIEND_CMP_XCHG_FAILURE_INTERNAL) <<(int)info.category;
	}

	const auto category = static_cast<FriendBox::Category>(req.category);
	if(info.category != category){
		if(category != FriendBox::CAT_DELETED){
			std::vector<FriendBox::FriendInfo> friends;
			friend_box->get_by_category(friends, category);
			if(friends.size() >= req.max_count){
				return Response(Msg::ERR_FRIEND_LIST_FULL_INTERNAL);
			}

			const auto utc_now = Poseidon::get_utc_time();

			info.category     = category;
			info.metadata     = std::move(req.metadata);
			info.updated_time = utc_now;
			friend_box->set(std::move(info));
		} else {
			if(!friend_box->remove(account_uuid)){
				return Response(Msg::ERR_NO_SUCH_FRIEND) <<account_uuid;
			}
		}
	}

	return Response();
}

CONTROLLER_SERVLET(Msg::TS_FriendPeerCompareExchangeResult, controller, req){
	const auto account_uuid = AccountUuid(req.account_uuid);
	const auto account = AccountMap::get(account_uuid);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_ACCOUNT) <<account_uuid;
	}

	const auto transaction_box = FriendBoxMap::get(account_uuid);
	if(!transaction_box){
		return Response(Msg::ERR_CONTROLLER_TOKEN_NOT_ACQUIRED) <<account_uuid;
	}

	const auto transaction_uuid = Poseidon::Uuid(req.transaction_uuid);

	transaction_box->set_async_request_result(transaction_uuid, std::make_pair(req.err_code, std::move(req.err_msg)));

	return Response();
}

}
