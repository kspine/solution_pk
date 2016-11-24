#include "precompiled.hpp"
#include "friend_box.hpp"
#include "msg/sc_friend.hpp"
#include "mysql/friend.hpp"
#include "singletons/player_session_map.hpp"
#include "player_session.hpp"
#include "singletons/account_map.hpp"
#include <poseidon/job_promise.hpp>

namespace EmperyCenter {

namespace {
	void fill_friend_info(FriendBox::FriendInfo &info, const boost::shared_ptr<MySql::Center_Friend> &obj){
		PROFILE_ME;

		info.friend_uuid  = AccountUuid(obj->unlocked_get_friend_uuid());
		info.category     = FriendBox::Category(obj->get_category());
		info.metadata     = obj->unlocked_get_metadata();
		info.updated_time = obj->get_updated_time();
		info.relation     = FriendBox::RelationType(obj->get_relation());
	}

	void fill_friend_message(Msg::SC_FriendChanged &msg, const boost::shared_ptr<MySql::Center_Friend> &obj,bool online){
		PROFILE_ME;

		msg.friend_uuid = obj->unlocked_get_friend_uuid().to_string();
		
		msg.category    = obj->get_category();
		msg.metadata    = obj->unlocked_get_metadata();
		msg.updated_time = obj->get_updated_time();
		msg.online       = online;
		if(FriendBox::RelationType(obj->get_relation()) == FriendBox::RT_BLACKLIST){
			msg.category = FriendBox::CAT_BLACKLIST;
		}
	}
}

FriendBox::FriendBox(AccountUuid account_uuid,
	const std::vector<boost::shared_ptr<MySql::Center_Friend>> &friends)
	: m_account_uuid(account_uuid)
{
	for(auto it = friends.begin(); it != friends.end(); ++it){
		const auto &obj = *it;
		auto &map = m_friends[Category(obj->get_category())];
		map.emplace(AccountUuid(obj->unlocked_get_friend_uuid()), obj);
	}
}
FriendBox::~FriendBox(){
}

void FriendBox::pump_status(){
	PROFILE_ME;

	//
}

FriendBox::FriendInfo FriendBox::get(AccountUuid friend_uuid) const {
	PROFILE_ME;

	FriendInfo info = { };
	info.friend_uuid = friend_uuid;
	for(auto cit = m_friends.begin(); cit != m_friends.end(); ++cit){
		const auto it = cit->second.find(friend_uuid);
		if(it == cit->second.end()){
			continue;
		}
		fill_friend_info(info, it->second);
		return info;
	}
	return info;
}
void FriendBox::get_all(std::vector<FriendBox::FriendInfo> &ret) const {
	PROFILE_ME;

	std::size_t size = 0;
	for(auto cit = m_friends.begin(); cit != m_friends.end(); ++cit){
		size += cit->second.size();
	}
	ret.reserve(ret.size() + size);
	for(auto cit = m_friends.begin(); cit != m_friends.end(); ++cit){
		for(auto it = cit->second.begin(); it != cit->second.end(); ++it){
			FriendInfo info;
			fill_friend_info(info, it->second);
			ret.emplace_back(std::move(info));
		}
	}
}
void FriendBox::get_by_category(std::vector<FriendInfo> &ret, FriendBox::Category category) const {
	PROFILE_ME;

	const auto cit = m_friends.find(category);
	if(cit == m_friends.end()){
		return;
	}
	ret.reserve(ret.size() + cit->second.size());
	for(auto it = cit->second.begin(); it != cit->second.end(); ++it){
		FriendInfo info;
		fill_friend_info(info, it->second);
		ret.emplace_back(std::move(info));
	}
}
void FriendBox::set(FriendBox::FriendInfo info){
	PROFILE_ME;

	const auto friend_uuid = info.friend_uuid;
	const auto category = info.category;

	auto &map = m_friends[category];

	auto it = map.find(friend_uuid);
	if(it == map.end()){
		map.reserve(map.size() + 1);

		boost::shared_ptr<MySql::Center_Friend> obj;
		for(auto cit = m_friends.begin(); cit != m_friends.end(); ++cit){
			if(cit->first == category){
				continue;
			}
			it = cit->second.find(friend_uuid);
			if(it == cit->second.end()){
				continue;
			}
			obj = std::move(it->second);
			cit->second.erase(it);
			break;
		}
		if(!obj){
			obj = boost::make_shared<MySql::Center_Friend>(get_account_uuid().get(), friend_uuid.get(),
				0, std::string(), 0,0);
			obj->async_save(true);
		}
		it = map.emplace(friend_uuid, obj).first;
	}
	const auto &obj = it->second;

	obj->set_category(static_cast<unsigned>(category));
	obj->set_metadata(std::move(info.metadata));
	obj->set_updated_time(info.updated_time);
	obj->set_relation(info.relation);

	const auto session = PlayerSessionMap::get(get_account_uuid());
	if(session){
		try {
			AccountMap::cached_synchronize_account_with_player_all(friend_uuid, session);
			bool friend_online = (PlayerSessionMap::get(friend_uuid) != NULL);
			Msg::SC_FriendChanged msg;
			fill_friend_message(msg, obj,friend_online);
			LOG_EMPERY_CENTER_FATAL(msg);
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
}
bool FriendBox::remove(AccountUuid friend_uuid) noexcept {
	PROFILE_ME;

	for(auto cit = m_friends.begin(); cit != m_friends.end(); ++cit){
		const auto it = cit->second.find(friend_uuid);
		if(it == cit->second.end()){
			continue;
		}
		const auto obj = std::move(it->second);
		cit->second.erase(it);

		obj->set_category(CAT_DELETED);

		const auto session = PlayerSessionMap::get(get_account_uuid());
		if(session){
			try {
				AccountMap::cached_synchronize_account_with_player_all(friend_uuid, session);
				bool friend_online = (PlayerSessionMap::get(friend_uuid) != NULL);
				Msg::SC_FriendChanged msg;
				fill_friend_message(msg, obj,friend_online);
				LOG_EMPERY_CENTER_FATAL(msg);
				session->send(msg);
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
				session->shutdown(e.what());
			}
		}

		return true;
	}
	return false;
}

void FriendBox::synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	for(auto cit = m_friends.begin(); cit != m_friends.end(); ++cit){
		for(auto it = cit->second.begin(); it != cit->second.end(); ++it){
			const auto friend_uuid = AccountUuid(it->second->unlocked_get_friend_uuid());
			AccountMap::cached_synchronize_account_with_player_all(friend_uuid, session);
			bool friend_online = (PlayerSessionMap::get(friend_uuid) != NULL);
			Msg::SC_FriendChanged msg;
			fill_friend_message(msg, it->second,friend_online);
			LOG_EMPERY_CENTER_FATAL(msg);
			session->send(msg);
		}
	}
}

Poseidon::Uuid FriendBox::create_async_request(boost::shared_ptr<Poseidon::JobPromise> promise,
	boost::shared_ptr<std::pair<long, std::string>> result)
{
	PROFILE_ME;

	auto transaction_uuid = Poseidon::Uuid::random();
	AsyncRequestResultControl control = { std::move(promise), std::move(result) };
	m_async_requests.emplace(transaction_uuid, std::move(control));
	LOG_EMPERY_CENTER_DEBUG("Friend box: Created async request: account_uuid = ", get_account_uuid(),
		", transaction_uuid = ", transaction_uuid);
	return transaction_uuid;
}
bool FriendBox::set_async_request_result(Poseidon::Uuid transaction_uuid, std::pair<long, std::string> &&result){
	PROFILE_ME;

	const auto it = m_async_requests.find(transaction_uuid);
	if(it == m_async_requests.end()){
		LOG_EMPERY_CENTER_DEBUG("Friend box: Async request not found: account_uuid = ", get_account_uuid(),
			", transaction_uuid = ", transaction_uuid);
		return false;
	}
	if(it->second.promise->is_satisfied()){
		LOG_EMPERY_CENTER_DEBUG("Friend box: Async request result already set: account_uuid = ", get_account_uuid(),
			", transaction_uuid = ", transaction_uuid);
		return false;
	}
	LOG_EMPERY_CENTER_DEBUG("Friend box: Set async request: account_uuid = ", get_account_uuid(),
		", transaction_uuid = ", transaction_uuid, ", err_code = ", result.first, ", err_msg = ", result.second);
	it->second.promise->set_success();
	*it->second.result = std::move(result);
	m_async_requests.erase(it);
	return true;
}
bool FriendBox::remove_async_request(Poseidon::Uuid transaction_uuid) noexcept {
	PROFILE_ME;

	const auto it = m_async_requests.find(transaction_uuid);
	if(it == m_async_requests.end()){
		LOG_EMPERY_CENTER_DEBUG("Friend box: Async request not found: account_uuid = ", get_account_uuid(),
			", transaction_uuid = ", transaction_uuid);
		return false;
	}
	LOG_EMPERY_CENTER_DEBUG("Friend box: Deleted async request: account_uuid = ", get_account_uuid(),
		", transaction_uuid = ", transaction_uuid);
	m_async_requests.erase(it);
	return true;
}

}
