#include "precompiled.hpp"
#include "chat_message.hpp"
#include "player_session.hpp"
#include "msg/sc_chat.hpp"
#include "singletons/account_map.hpp"

namespace EmperyCenter {

ChatMessage::ChatMessage(ChatChannelId channel, ChatMessageTypeId type, LanguageId language_id, boost::uint64_t created_time,
	AccountUuid from_account_uuid, std::vector<std::pair<ChatMessageSlotId, std::string>> segments)
	: m_chat_message_uuid(Poseidon::Uuid::random())
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
		AccountMap::combined_send_attributes_to_client(from_account_uuid, session);
	}

	Msg::SC_ChatMessageData msg;
	msg.chat_message_uuid   = get_chat_message_uuid().str();
	msg.channel             = get_channel().get();
	msg.type                = get_type().get();
	msg.language_id         = get_language_id().get();
	msg.created_time        = get_created_time();
	msg.from_account_uuid   = from_account_uuid.str();
	msg.segments.reserve(m_segments.size());
	for(auto it = m_segments.begin(); it != m_segments.end(); ++it){
	msg.segments.emplace_back();
	auto &segment = msg.segments.back();
	segment.slot   = it->first.get();
	segment.value  = it->second;
	}
	session->send(msg);
}

}
