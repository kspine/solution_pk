#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/cs_friend.hpp"
#include "../msg/sc_friend.hpp"
#include "../msg/st_friend.hpp"
#include "../msg/err_friend.hpp"
#include "../singletons/friend_box_map.hpp"
#include "../friend_box.hpp"
#include "../singletons/controller_client.hpp"
#include "../data/global.hpp"
#include <poseidon/job_promise.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>

namespace EmperyCenter {

PLAYER_SERVLET(Msg::CS_FriendGetAll, account, session, /* req */){
	const auto friend_box = FriendBoxMap::require(account->get_account_uuid());
	friend_box->pump_status();

	friend_box->synchronize_with_player(session);

	return Response();
}

namespace {
	std::pair<long, std::string> interserver_compare_exchange_and_wait(const boost::shared_ptr<ControllerClient> &controller,
		const boost::shared_ptr<FriendBox> &transaction_box, AccountUuid friend_uuid,
		std::initializer_list<FriendBox::Category> catagories_expected, FriendBox::Category category, std::uint64_t max_count)
	{
		PROFILE_ME;

		const auto promise = boost::make_shared<Poseidon::JobPromise>();
		const auto tresult = boost::make_shared<std::pair<long, std::string>>(12345678, std::string());
		const auto transaction_uuid = transaction_box->create_async_request(promise, tresult);

		try {
			Msg::ST_FriendPeerCompareExchange treq;
			treq.account_uuid      = transaction_box->get_account_uuid().str();
			treq.transaction_uuid  = transaction_uuid.to_string();
			treq.friend_uuid       = friend_uuid.str();
			treq.categories_expected.reserve(catagories_expected.size());
			for(auto it = catagories_expected.begin(); it != catagories_expected.end(); ++it){
				auto &elem = *treq.categories_expected.emplace(treq.categories_expected.end());
				elem.category_expected = static_cast<unsigned>(*it);
			}
			treq.category          = static_cast<unsigned>(category);
			treq.max_count         = max_count;
			if(!controller->send(treq)){
				LOG_EMPERY_CENTER_WARNING("Lost connection to center server.");
				DEBUG_THROW(Exception, sslit("Lost connection to center server"));
			}
			Poseidon::JobDispatcher::yield(promise, false);

			return std::move(*tresult);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			transaction_box->remove_async_request(transaction_uuid);
			throw;
		}
	}
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

	auto tresult = interserver_compare_exchange_and_wait(controller, friend_box, friend_uuid,
		{ FriendBox::CAT_DELETED, FriendBox::CAT_REQUESTING }, FriendBox::CAT_REQUESTED, max_number_of_friends_requested);
	if(tresult.first != Msg::ST_OK){
		LOG_EMPERY_CENTER_DEBUG("Controller server response: code = ", tresult.first, ", msg = ", tresult.second);
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

	auto tresult = interserver_compare_exchange_and_wait(controller, friend_box, friend_uuid,
		{ FriendBox::CAT_REQUESTING }, FriendBox::CAT_FRIEND, max_number_of_friends);
	if(tresult.first != Msg::ST_OK){
		LOG_EMPERY_CENTER_DEBUG("Controller server response: code = ", tresult.first, ", msg = ", tresult.second);
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

	auto tresult = interserver_compare_exchange_and_wait(controller, friend_box, friend_uuid,
		{ FriendBox::CAT_REQUESTING }, FriendBox::CAT_DELETED, 0);
	if(tresult.first != Msg::ST_OK){
		LOG_EMPERY_CENTER_DEBUG("Controller server response: code = ", tresult.first, ", msg = ", tresult.second);
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

	auto tresult = interserver_compare_exchange_and_wait(controller, friend_box, friend_uuid,
		{ FriendBox::CAT_FRIEND }, FriendBox::CAT_DELETED, 0);
	if(tresult.first != Msg::ST_OK){
		LOG_EMPERY_CENTER_DEBUG("Controller server response: code = ", tresult.first, ", msg = ", tresult.second);
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

	auto tresult = interserver_compare_exchange_and_wait(controller, friend_box, friend_uuid,
		{ FriendBox::CAT_REQUESTED }, FriendBox::CAT_DELETED, 0);
	if(tresult.first != Msg::ST_OK){
		LOG_EMPERY_CENTER_DEBUG("Controller server response: code = ", tresult.first, ", msg = ", tresult.second);
		if(tresult.first == Msg::ERR_FRIEND_CMP_XCHG_FAILURE_INTERNAL){
			return Response(Msg::ERR_FRIEND_NOT_REQUESTING) <<friend_uuid;
		}
		return std::move(tresult);
	}

	friend_box->remove(friend_uuid);

	return Response();
}

PLAYER_SERVLET(Msg::CS_FriendRandom, account, session, req){
	static constexpr unsigned MAX_RANDOM_FRIEND_COUNT = 32;
	const auto max_count = std::min<std::size_t>(req.max_count, MAX_RANDOM_FRIEND_COUNT);

	const auto account_uuid = account->get_account_uuid();
	const auto friend_box = FriendBoxMap::require(account_uuid);
	friend_box->pump_status();

	std::vector<AccountUuid> random_list;
	FriendBoxMap::random(random_list, max_count, friend_box);

	Msg::SC_FriendRandomList msg;
	msg.friends.reserve(random_list.size());
	for(auto it = random_list.begin(); it != random_list.end(); ++it){
		auto &elem = *msg.friends.emplace(msg.friends.end());

		const auto friend_uuid = *it;
		const auto friend_account = AccountMap::require(friend_uuid);

		AccountMap::cached_synchronize_account_with_player_all(friend_uuid, session);

		elem.friend_uuid = friend_uuid.str();
	}
	session->send(msg);

	return Response();
}

PLAYER_SERVLET(Msg::CS_FriendPrivateMessage, account, session, req){
	const auto friend_uuid = AccountUuid(req.friend_uuid);

	const auto controller = ControllerClient::require();

	const auto account_uuid = account->get_account_uuid();
	const auto friend_box = FriendBoxMap::require(account_uuid);
	friend_box->pump_status();

	const auto info = friend_box->get(friend_uuid);
	if(info.category != FriendBox::CAT_FRIEND){
		return Response(Msg::ERR_NO_SUCH_FRIEND) <<friend_uuid;
	}

	const auto promise = boost::make_shared<Poseidon::JobPromise>();
	const auto tresult = boost::make_shared<std::pair<long, std::string>>(12345678, std::string());
	const auto transaction_uuid = friend_box->create_async_request(promise, tresult);

	try {
		Msg::ST_FriendPrivateMessage treq;
		treq.account_uuid      = account_uuid.str();
		treq.transaction_uuid  = transaction_uuid.to_string();
		treq.friend_uuid       = friend_uuid.str();
		treq.language_id       = req.language_id;
		treq.segments.reserve(req.segments.size());
		for(auto it = req.segments.begin(); it != req.segments.end(); ++it){
			auto &elem = *treq.segments.emplace(treq.segments.end());
			elem.slot  = it->slot;
			elem.value = std::move(it->value);
		}
		if(!controller->send(treq)){
			LOG_EMPERY_CENTER_WARNING("Lost connection to center server.");
			DEBUG_THROW(Exception, sslit("Lost connection to center server"));
		}
		Poseidon::JobDispatcher::yield(promise, false);
	} catch(std::exception &e){
		LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
		friend_box->remove_async_request(transaction_uuid);
		throw;
	}
	if(tresult->first != Msg::ST_OK){
		LOG_EMPERY_CENTER_DEBUG("Controller server response: code = ", tresult->first, ", msg = ", tresult->second);
		return std::move(*tresult);
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_FriendSearchByNick, account, session, req){
	auto &nick = req.nick;

	std::vector<boost::shared_ptr<Account>> accounts;
	AccountMap::get_by_nick(accounts, nick);
	Msg::SC_FriendSearchByNickResult msg;
	msg.results.reserve(accounts.size());
	for(auto it = accounts.begin(); it != accounts.end(); ++it){
		const auto &other_account = *it;
		const auto other_account_uuid = other_account->get_account_uuid();
		AccountMap::cached_synchronize_account_with_player_all(other_account_uuid, session);
		auto &elem = *msg.results.emplace(msg.results.end());
		elem.other_uuid = other_account_uuid.str();
	}
	session->send(msg);
	return Response();
}


}
