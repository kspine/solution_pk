#include "precompiled.hpp"
#include "chat_message.hpp"
#include "msg/sc_chat.hpp"
#include "player_session.hpp"
#include "singletons/account_map.hpp"

namespace EmperyCenter {

ChatMessage::ChatMessage(unsigned channel, unsigned type, LanguageId language_id,
	AccountUuid from_account_uuid, boost::uint64_t sent_time, std::vector<std::pair<unsigned, std::string>> segments)
	: m_chat_message_uuid(Poseidon::Uuid::random())
	, m_channel(channel), m_type(type), m_language_id(language_id)
	, m_from_account_uuid(from_account_uuid), m_sent_time(sent_time), m_segments(std::move(segments))
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
	msg.channel             = get_channel();
	msg.type                = get_type();
	msg.language_id         = get_language_id().get();
	msg.from_account_uuid   = from_account_uuid.str();
	msg.segments.reserve(m_segments.size());
	for(auto it = m_segments.begin(); it != m_segments.end(); ++it){
		msg.segments.emplace_back();
		auto &segment = msg.segments.back();
		segment.slot   = it->first;
		segment.value  = it->second;
	}
	session->send(msg);
}

}
