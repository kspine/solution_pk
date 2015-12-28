#include "precompiled.hpp"
#include "chat_message.hpp"
#include "player_session.hpp"
#include "msg/sc_chat.hpp"
#include "singletons/player_session_map.hpp"
#include "singletons/account_map.hpp"
#include "chat_message_slot_ids.hpp"

namespace EmperyCenter {

ChatMessage::ChatMessage(ChatMessageUuid chat_message_uuid,
	ChatChannelId channel, ChatMessageTypeId type, LanguageId language_id, std::uint64_t created_time,
	AccountUuid from_account_uuid, std::vector<std::pair<ChatMessageSlotId, std::string>> segments)
	: m_chat_message_uuid(chat_message_uuid)
	, m_channel(channel), m_type(type), m_language_id(language_id), m_created_time(created_time)
	, m_from_account_uuid(from_account_uuid), m_segments(std::move(segments))
{
}
ChatMessage::~ChatMessage(){
}

void ChatMessage::synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	const auto from_account_uuid = get_from_account_uuid();
	if(from_account_uuid){
		AccountMap::cached_synchronize_account_with_player(from_account_uuid, session);
	}

	presend_chat_message_segments(m_segments, session);

	Msg::SC_ChatMessageData msg;
	msg.chat_message_uuid   = get_chat_message_uuid().str();
	msg.channel             = get_channel().get();
	msg.type                = get_type().get();
	msg.language_id         = get_language_id().get();
	msg.created_time        = get_created_time();
	msg.from_account_uuid   = from_account_uuid.str();
	msg.segments.reserve(m_segments.size());
	for(auto it = m_segments.begin(); it != m_segments.end(); ++it){
		auto &segment = *msg.segments.emplace(msg.segments.end());
		segment.slot   = it->first.get();
		segment.value  = it->second;
	}
	session->send(msg);
}

void presend_chat_message_segments(const std::vector<std::pair<ChatMessageSlotId, std::string>> &segments,
	const boost::shared_ptr<PlayerSession> &session)
{
	PROFILE_ME;

	for(auto it = segments.begin(); it != segments.end(); ++it){
		const auto slot_id = it->first;
		const auto &value = it->second;

		if(slot_id == ChatMessageSlotIds::ID_TAXER){
			AccountMap::cached_synchronize_account_with_player(AccountUuid(value), session);
		}
	}
}

}
