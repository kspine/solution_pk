#ifndef EMPERY_CENTER_SINGLETONS_CHAT_MESSAGE_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_CHAT_MESSAGE_MAP_HPP_

#include <boost/shared_ptr.hpp>
#include <vector>
#include "../id_types.hpp"

namespace EmperyCenter {

class ChatMessage;

struct ChatMessageMap {
	static boost::shared_ptr<ChatMessage> get(ChatMessageUuid chat_message_uuid);
	static boost::shared_ptr<ChatMessage> require(ChatMessageUuid chat_message_uuid);
	static void get_all_by_channel(std::vector<boost::shared_ptr<ChatMessage>> &ret, unsigned channel);

	static void insert(const boost::shared_ptr<ChatMessage> &message);
	static void remove(ChatMessageUuid chat_message_uuid) noexcept;

private:
	ChatMessageMap() = delete;
};

}

#endif
