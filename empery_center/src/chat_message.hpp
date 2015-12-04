#ifndef EMPERY_CENTER_CHAT_MESSAGE_HPP_
#define EMPERY_CENTER_CHAT_MESSAGE_HPP_

#include <poseidon/cxx_util.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/cstdint.hpp>
#include <vector>
#include <string>
#include "id_types.hpp"

namespace EmperyCenter {

class PlayerSession;

class ChatMessage : NONCOPYABLE {
public:
	enum {
		// Channel
		CHAN_CLUSTER    = 0,
		CHAN_SYSTEM     = 1,
		CHAN_TRADE      = 2,
		CHAN_ALLIANCE   = 3,

		// Type
		T_PLAIN         = 0,

		// Slot
		SLOT_TEXT       = 0,
		SLOT_SMILEY     = 1,
		SLOT_VOICE      = 2,
	};

private:
	const ChatMessageUuid m_chat_message_uuid;

	unsigned m_channel;
	unsigned m_type;
	LanguageId m_language_id;

	AccountUuid m_from_account_uuid;
	boost::uint64_t m_sent_time;
	std::vector<std::pair<unsigned, std::string>> m_segments;

public:
	ChatMessage(unsigned channel, unsigned type, LanguageId language_id,
		AccountUuid from_account_uuid, boost::uint64_t sent_time, std::vector<std::pair<unsigned, std::string>> segments);
	~ChatMessage();

public:
	ChatMessageUuid get_chat_message_uuid() const {
		return m_chat_message_uuid;
	}

	unsigned get_channel() const {
		return m_channel;
	}
	unsigned get_type() const {
		return m_type;
	}
	LanguageId get_language_id() const {
		return m_language_id;
	}

	AccountUuid get_from_account_uuid() const {
		return m_from_account_uuid;
	}
	boost::uint64_t get_sent_time() const {
		return m_sent_time;
	}
	const std::vector<std::pair<unsigned, std::string>> &get_segments() const {
		return m_segments;
	}

	void synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const;
};

inline void synchronize_chat_message_with_player(const boost::shared_ptr<const ChatMessage> &chat_message,
	const boost::shared_ptr<PlayerSession> &session)
{
	chat_message->synchronize_with_player(session);
}
inline void synchronize_chat_message_with_player(const boost::shared_ptr<ChatMessage> &chat_message,
	const boost::shared_ptr<PlayerSession> &session)
{
	chat_message->synchronize_with_player(session);
}

}

#endif
