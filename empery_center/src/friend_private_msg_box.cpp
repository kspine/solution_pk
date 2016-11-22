#include "precompiled.hpp"
#include "friend_private_msg_box.hpp"
#include "mysql/friend.hpp"
#include "data/global.hpp"
#include "singletons/player_session_map.hpp"
#include "player_session.hpp"

namespace EmperyCenter {

namespace {
	void fill_friend_private_msg_info(FriendPrivateMsgBox::PrivateMsgInfo &info, const boost::shared_ptr<MySql::Center_FriendPrivateMsgRecent> &obj){
		PROFILE_ME;

		info.auto_uuid              = obj->unlocked_get_auto_uuid();
		info.timestamp              = obj->get_timestamp();
		info.friend_account_uuid    = AccountUuid(obj->unlocked_get_friend_account_uuid());
		info.msg_uuid               = FriendPrivateMsgUuid(obj->unlocked_get_msg_uuid());
		info.sender                 = obj->get_sender();
		info.read                   = obj->get_read();
		info.deleted                = obj->get_deleted();
	}
}

FriendPrivateMsgBox::FriendPrivateMsgBox(AccountUuid account_uuid,
	const std::vector<boost::shared_ptr<MySql::Center_FriendPrivateMsgRecent>> &recent_msgs)
	: m_account_uuid(account_uuid)
{
	for(auto it = recent_msgs.begin(); it != recent_msgs.end(); ++it){
		const auto &recent_msg = *it;
		m_recent_msgs.emplace_back(recent_msg);
	}
	std::sort(m_recent_msgs.begin(), m_recent_msgs.end(),
		[](const boost::shared_ptr<MySql::Center_FriendPrivateMsgRecent> &lhs, const boost::shared_ptr<MySql::Center_FriendPrivateMsgRecent> &rhs){
			return lhs->unlocked_get_auto_uuid() < rhs->unlocked_get_auto_uuid();
		});
}
FriendPrivateMsgBox::~FriendPrivateMsgBox(){
}

void FriendPrivateMsgBox::pump_status(){
	PROFILE_ME;

	const auto expiry_days = Data::Global::as_unsigned(Data::Global::SLOT_FRIEND_RECENT_EXPIRY_DAYS);
	const auto expired_before = Poseidon::get_utc_time_from_local((Poseidon::get_local_time() / 86400000 - expiry_days) * 86400000);
	for(;;){
		if(m_recent_msgs.empty()){
			break;
		}
		const auto obj = m_recent_msgs.front();
		if(obj->get_timestamp() >= expired_before){
			break;
		}
		LOG_EMPERY_CENTER_DEBUG("Removing expired friend recent_msg: account_uuid = ", get_account_uuid(),
			", auto_uuid = ", obj->unlocked_get_auto_uuid());
		m_recent_msgs.pop_front();
		obj->set_deleted(true);
	}
}

void FriendPrivateMsgBox::get_all(std::vector<FriendPrivateMsgBox::PrivateMsgInfo> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_recent_msgs.size());
	for(auto it = m_recent_msgs.begin(); it != m_recent_msgs.end(); ++it){
		PrivateMsgInfo info;
		fill_friend_private_msg_info(info, *it);
		ret.emplace_back(std::move(info));
	}
}
void FriendPrivateMsgBox::get_offline(std::vector<PrivateMsgInfo> &ret) const{
	PROFILE_ME;

	ret.reserve(ret.size() + m_recent_msgs.size());
	for(auto it = m_recent_msgs.begin(); it != m_recent_msgs.end(); ++it){
		PrivateMsgInfo info;
		auto &obj = *it;
		if(obj->get_read()){
			continue;
		}
		obj->set_read(true);
		fill_friend_private_msg_info(info, *it);
		ret.emplace_back(std::move(info));
	}
}

void FriendPrivateMsgBox::get_recent_contact(boost::container::flat_map<AccountUuid, std::uint64_t> &recent_contact) const{
	PROFILE_ME;

	recent_contact.reserve(recent_contact.size() + m_recent_msgs.size());
	for(auto it = m_recent_msgs.begin(); it != m_recent_msgs.end(); ++it){
		auto &obj = *it;
		auto friend_account_uuid = AccountUuid(obj->get_friend_account_uuid());
		auto itc = recent_contact.find(friend_account_uuid);
		if(itc == recent_contact.end()){
			recent_contact.emplace(friend_account_uuid,obj->get_timestamp());
		}else{
			if(obj->get_timestamp() > itc->second){
				itc->second = obj->get_timestamp();
			}
			
		}
	}
}

void FriendPrivateMsgBox::push(std::uint64_t timestamp,AccountUuid friend_account_uuid,FriendPrivateMsgUuid msg_uuid,bool sender,bool read,bool deleted)
{
	PROFILE_ME;

	const auto obj = boost::make_shared<MySql::Center_FriendPrivateMsgRecent>(Poseidon::Uuid::random(),get_account_uuid().get(), timestamp,friend_account_uuid.get(),msg_uuid.get(),sender,read,deleted);
	obj->async_save(true);
	m_recent_msgs.emplace_back(obj);

	const auto max_record_count = Data::Global::as_unsigned(Data::Global::SLOT_MAX_FRIEND_RECORD_COUNT);
	while(m_recent_msgs.size() > max_record_count){
		const auto obj = std::move(m_recent_msgs.front());
		m_recent_msgs.pop_front();
		obj->set_deleted(true);
	}
}

}
