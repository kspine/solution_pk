#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/cs_friend.hpp"
#include "../msg/st_friend.hpp"
#include "../msg/err_friend.hpp"
#include "../singletons/friend_box_map.hpp"
#include "../friend_box.hpp"
#include "../singletons/controller_client.hpp"
#include "../data/global.hpp"

namespace EmperyCenter {

PLAYER_SERVLET(Msg::CS_FriendGetAll, account, session, /* req */){
	const auto friend_box = FriendBoxMap::require(account->get_account_uuid());
	friend_box->pump_status();

	friend_box->synchronize_with_player(session);

	return Response();
}

PLAYER_SERVLET(Msg::CS_FriendRequest, account, session, req){
	const auto friend_uuid = AccountUuid(req.friend_uuid);

	const auto controller = ControllerClient::require();

	const auto account_uuid = account->get_account_uuid();
	const auto friend_box = FriendBoxMap::require(account_uuid);
	friend_box->pump_status();

	auto info = friend_box->get(friend_uuid);
	if(info.category == FriendBox::CAT_FRIEND){
		return Response(Msg::ERR_ALREADY_A_FRIEND) <<friend_uuid;
	}
	if(info.category == FriendBox::CAT_REQUESTING){
		return Response(Msg::ERR_FRIEND_REQUESTING) <<friend_uuid;
	}
//	if(info.category == FriendBox::CAT_REQUESTED){
//		return Response(Msg::ERR_FRIEND_REQUESTED) <<friend_uuid;
//	}

	const auto max_number_of_friends_requesting = Data::Global::as_unsigned(Data::Global::SLOT_MAX_NUMBER_OF_FRIENDS_REQUESTING);
	const auto max_number_of_friends_requested  = Data::Global::as_unsigned(Data::Global::SLOT_MAX_NUMBER_OF_FRIENDS_REQUESTED);

	std::vector<FriendBox::FriendInfo> friends;
	friend_box->get_by_category(friends, FriendBox::CAT_REQUESTING);
	if(friends.size() >= max_number_of_friends_requesting){
		return Response(Msg::ERR_FRIEND_REQUESTING_LIST_FULL) <<max_number_of_friends_requesting;
	}

	Msg::ST_FriendCompareExchangeSync treq;
	treq.account_uuid      = friend_uuid.str();
	treq.friend_uuid       = account_uuid.str();
	treq.categories_expected.resize(2);
	treq.categories_expected.at(0).category_expected = static_cast<unsigned>(FriendBox::CAT_DELETED);
	treq.categories_expected.at(1).category_expected = static_cast<unsigned>(FriendBox::CAT_REQUESTING);
	treq.category          = static_cast<unsigned>(FriendBox::CAT_REQUESTED);
	treq.max_count         = max_number_of_friends_requested;
	auto tresult = controller->send_and_wait(treq);
	if(tresult.first != Msg::ST_OK){
		LOG_EMPERY_CENTER_DEBUG("Controller server response: treq = ", treq, ", code = ", tresult.first, ", msg = ", tresult.second);
		if(tresult.first == Msg::ERR_FRIEND_CMP_XCHG_FAILURE_INTERNAL){
			return Response(Msg::ERR_FRIEND_BLACKLISTED) <<friend_uuid;
		}
		if(tresult.first == Msg::ERR_FRIEND_LIST_FULL_INTERNAL){
			return Response(Msg::ERR_FRIEND_REQUESTED_LIST_FULL_PEER) <<max_number_of_friends_requested;
		}
		return std::move(tresult);
	}

	const auto utc_now = Poseidon::get_utc_time();

	info.category     = FriendBox::CAT_REQUESTING;
	info.updated_time = utc_now;
	friend_box->set(std::move(info));

	return Response();
}

PLAYER_SERVLET(Msg::CS_FriendAccept, account, session, req){
	const auto friend_uuid = AccountUuid(req.friend_uuid);

	const auto controller = ControllerClient::require();

	const auto account_uuid = account->get_account_uuid();
	const auto friend_box = FriendBoxMap::require(account_uuid);
	friend_box->pump_status();

	auto info = friend_box->get(friend_uuid);
	if(info.category != FriendBox::CAT_REQUESTED){
		return Response(Msg::ERR_FRIEND_NOT_REQUESTED) <<friend_uuid;
	}

	const auto max_number_of_friends = Data::Global::as_unsigned(Data::Global::SLOT_MAX_NUMBER_OF_FRIENDS);

	std::vector<FriendBox::FriendInfo> friends;
	friend_box->get_by_category(friends, FriendBox::CAT_FRIEND);
	if(friends.size() >= max_number_of_friends){
		return Response(Msg::ERR_FRIEND_LIST_FULL) <<max_number_of_friends;
	}

	Msg::ST_FriendCompareExchangeSync treq;
	treq.account_uuid      = friend_uuid.str();
	treq.friend_uuid       = account_uuid.str();
	treq.categories_expected.resize(1);
	treq.categories_expected.at(0).category_expected = static_cast<unsigned>(FriendBox::CAT_REQUESTED);
	treq.category          = static_cast<unsigned>(FriendBox::CAT_FRIEND);
	treq.max_count         = max_number_of_friends;
	auto tresult = controller->send_and_wait(treq);
	if(tresult.first != Msg::ST_OK){
		LOG_EMPERY_CENTER_DEBUG("Controller server response: treq = ", treq, ", code = ", tresult.first, ", msg = ", tresult.second);
		if(tresult.first == Msg::ERR_FRIEND_CMP_XCHG_FAILURE_INTERNAL){
			return Response(Msg::ERR_FRIEND_NOT_REQUESTED) <<friend_uuid;
		}
		if(tresult.first == Msg::ERR_FRIEND_LIST_FULL_INTERNAL){
			return Response(Msg::ERR_FRIEND_LIST_FULL_PEER) <<max_number_of_friends;
		}
		return std::move(tresult);
	}

	const auto utc_now = Poseidon::get_utc_time();

	info.category     = FriendBox::CAT_FRIEND;
	info.updated_time = utc_now;
	friend_box->set(std::move(info));

	return Response();
}

PLAYER_SERVLET(Msg::CS_FriendDecline, account, session, req){
	const auto friend_uuid = AccountUuid(req.friend_uuid);

	const auto controller = ControllerClient::require();

	const auto account_uuid = account->get_account_uuid();
	const auto friend_box = FriendBoxMap::require(account_uuid);
	friend_box->pump_status();

	auto info = friend_box->get(friend_uuid);
	if(info.category != FriendBox::CAT_REQUESTED){
		return Response(Msg::ERR_FRIEND_NOT_REQUESTED) <<friend_uuid;
	}

	Msg::ST_FriendCompareExchangeSync treq;
	treq.account_uuid      = friend_uuid.str();
	treq.friend_uuid       = account_uuid.str();
	treq.categories_expected.resize(1);
	treq.categories_expected.at(0).category_expected = static_cast<unsigned>(FriendBox::CAT_REQUESTING);
	treq.category          = static_cast<unsigned>(FriendBox::CAT_DELETED);
	auto tresult = controller->send_and_wait(treq);
	if(tresult.first != Msg::ST_OK){
		LOG_EMPERY_CENTER_DEBUG("Controller server response: treq = ", treq, ", code = ", tresult.first, ", msg = ", tresult.second);
		if(tresult.first == Msg::ERR_FRIEND_CMP_XCHG_FAILURE_INTERNAL){
			return Response(Msg::ERR_FRIEND_NOT_REQUESTED) <<friend_uuid;
		}
		return std::move(tresult);
	}

	friend_box->remove(friend_uuid);

	return Response();
}

PLAYER_SERVLET(Msg::CS_FriendDelete, account, session, req){
	const auto friend_uuid = AccountUuid(req.friend_uuid);

	const auto controller = ControllerClient::require();

	const auto account_uuid = account->get_account_uuid();
	const auto friend_box = FriendBoxMap::require(account_uuid);
	friend_box->pump_status();

	auto info = friend_box->get(friend_uuid);
	if(info.category != FriendBox::CAT_FRIEND){
		return Response(Msg::ERR_NO_SUCH_FRIEND) <<friend_uuid;
	}

	Msg::ST_FriendCompareExchangeSync treq;
	treq.account_uuid      = friend_uuid.str();
	treq.friend_uuid       = account_uuid.str();
	treq.categories_expected.resize(1);
	treq.categories_expected.at(0).category_expected = static_cast<unsigned>(FriendBox::CAT_FRIEND);
	treq.category          = static_cast<unsigned>(FriendBox::CAT_DELETED);
	auto tresult = controller->send_and_wait(treq);
	if(tresult.first != Msg::ST_OK){
		LOG_EMPERY_CENTER_DEBUG("Controller server response: treq = ", treq, ", code = ", tresult.first, ", msg = ", tresult.second);
	}

	friend_box->remove(friend_uuid);

	return Response();
}

PLAYER_SERVLET(Msg::CS_FriendCancelRequest, account, session, req){
	const auto friend_uuid = AccountUuid(req.friend_uuid);

	const auto controller = ControllerClient::require();

	const auto account_uuid = account->get_account_uuid();
	const auto friend_box = FriendBoxMap::require(account_uuid);
	friend_box->pump_status();

	auto info = friend_box->get(friend_uuid);
	if(info.category != FriendBox::CAT_REQUESTING){
		return Response(Msg::ERR_FRIEND_NOT_REQUESTING) <<friend_uuid;
	}

	Msg::ST_FriendCompareExchangeSync treq;
	treq.account_uuid      = friend_uuid.str();
	treq.friend_uuid       = account_uuid.str();
	treq.categories_expected.resize(1);
	treq.categories_expected.at(0).category_expected = static_cast<unsigned>(FriendBox::CAT_REQUESTED);
	treq.category          = static_cast<unsigned>(FriendBox::CAT_DELETED);
	auto tresult = controller->send_and_wait(treq);
	if(tresult.first != Msg::ST_OK){
		LOG_EMPERY_CENTER_DEBUG("Controller server response: treq = ", treq, ", code = ", tresult.first, ", msg = ", tresult.second);
		if(tresult.first == Msg::ERR_FRIEND_CMP_XCHG_FAILURE_INTERNAL){
			return Response(Msg::ERR_FRIEND_NOT_REQUESTING) <<friend_uuid;
		}
		return std::move(tresult);
	}

	friend_box->remove(friend_uuid);

	return Response();
}

}
