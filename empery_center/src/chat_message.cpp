#include "precompiled.hpp"
#include "chat_message.hpp"

namespace EmperyCenter {

ChatMessage::ChatMessage(Channel channel, Type type, LanguageId language_id,
	AccountUuid from_account_uuid, boost::uint64_t sent_time, std::vector<std::pair<Slot, std::string>> segments)
	: m_chat_message_uuid(Poseidon::Uuid::random())
	, m_channel(channel), m_type(type), m_language_id(language_id)
	, m_from_account_uuid(from_account_uuid), m_sent_time(sent_time), m_segments(std::move(segments))
{
}
ChatMessage::~ChatMessage(){
}

}
