#include "precompiled.hpp"
#include "friend_private_msg_data.hpp"
#include <poseidon/json.hpp>
#include "mysql/friend.hpp"
#include "player_session.hpp"
#include "msg/sc_mail.hpp"
#include "singletons/player_session_map.hpp"
#include "singletons/account_map.hpp"
#include "chat_message.hpp"

namespace EmperyCenter {

namespace {
	std::string encode_segments(const std::vector<std::pair<ChatMessageSlotId, std::string>> &segments){
		PROFILE_ME;

		if(segments.empty()){
			return { };
		}
		Poseidon::JsonArray root;
		for(auto it = segments.begin(); it != segments.end(); ++it){
			const auto slot = it->first;
			const auto &value = it->second;
			Poseidon::JsonArray elem;
			elem.emplace_back(slot.get());
			elem.emplace_back(value);
			root.emplace_back(std::move(elem));
		}
		std::ostringstream oss;
		root.dump(oss);
		return oss.str();
	}
	std::vector<std::pair<ChatMessageSlotId, std::string>> decode_segments(const std::string &str){
		PROFILE_ME;

		std::vector<std::pair<ChatMessageSlotId, std::string>> segments;
		if(str.empty()){
			return segments;
		}
		std::istringstream iss(str);
		auto root = Poseidon::JsonParser::parse_array(iss);
		segments.reserve(root.size());
		for(auto it = root.begin(); it != root.end(); ++it){
			auto elem = it->get<Poseidon::JsonArray>();
			auto slot = ChatMessageSlotId(elem.at(0).get<double>());
			auto value = std::move(elem.at(1)).get<std::string>();
			segments.emplace_back(slot, std::move(value));
		}
		return segments;
	}
}

FriendPrivateMsgData::FriendPrivateMsgData(FriendPrivateMsgUuid msg_uuid, LanguageId language_id, std::uint64_t created_time, AccountUuid from_account_uuid,std::vector<std::pair<ChatMessageSlotId, std::string>> segments)
	: m_obj(
		[&]{
			auto obj = boost::make_shared<MySql::Center_FriendPrivateMsgData>(msg_uuid.get(), language_id.get(), created_time, from_account_uuid.get(),encode_segments(segments));
			obj->async_save(true, true);
			return obj;
		}())
	, m_segments(std::move(segments))
{
}
FriendPrivateMsgData::FriendPrivateMsgData(boost::shared_ptr<MySql::Center_FriendPrivateMsgData> obj)
	: m_obj(std::move(obj))
	, m_segments(decode_segments(m_obj->unlocked_get_segments()))
{
}
FriendPrivateMsgData::~FriendPrivateMsgData(){
}

FriendPrivateMsgUuid FriendPrivateMsgData::get_msg_uuid() const {
	return FriendPrivateMsgUuid(m_obj->unlocked_get_msg_uuid());
}
LanguageId FriendPrivateMsgData::get_language_id() const {
	return LanguageId(m_obj->get_language_id());
}
std::uint64_t FriendPrivateMsgData::get_created_time() const {
	return m_obj->get_created_time();
}

AccountUuid FriendPrivateMsgData::get_from_account_uuid() const {
	return AccountUuid(m_obj->unlocked_get_from_account_uuid());
}
void FriendPrivateMsgData::set_from_account_uuid(AccountUuid account_uuid){
	m_obj->set_from_account_uuid(account_uuid.get());
}


const std::vector<std::pair<ChatMessageSlotId, std::string>> &FriendPrivateMsgData::get_segments() const {
	return m_segments;
}
void FriendPrivateMsgData::set_segments(std::vector<std::pair<ChatMessageSlotId, std::string>> segments){
	PROFILE_ME;

	auto str = encode_segments(segments);
	m_segments = std::move(segments);
	m_obj->set_segments(std::move(str));
}

void FriendPrivateMsgData::synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	/*
	const auto from_account_uuid = get_from_account_uuid();
	if(from_account_uuid){
		AccountMap::cached_synchronize_account_with_player_all(from_account_uuid, session);
	}

	presend_chat_message_segments(m_segments, session);

	Msg::SC_FriendPrivateMsgData msg;
	msg.mail_uuid         = get_mail_uuid().str();
	msg.language_id       = get_language_id().get();
	msg.created_time      = get_created_time();
	msg.type              = get_type().get();
	msg.from_account_uuid = from_account_uuid.str();
	msg.subject           = get_subject();
	msg.segments.reserve(m_segments.size());
	for(auto it = m_segments.begin(); it != m_segments.end(); ++it){
		auto &segment = *msg.segments.emplace(msg.segments.end());
		segment.slot  = it->first.get();
		segment.value = it->second;
	}
	msg.attachments.reserve(m_attachments.size());
	for(auto it = m_attachments.begin(); it != m_attachments.end(); ++it){
		auto &attachment = *msg.attachments.emplace(msg.attachments.end());
		attachment.item_id    = it->first.get();
		attachment.item_count = it->second;
	}
	session->send(msg);
	*/
}

}
