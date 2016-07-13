#ifndef EMPERY_CENTER_SINGLETONS_CHAT_BOX_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_CHAT_BOX_MAP_HPP_

#include <boost/shared_ptr.hpp>
#include "../id_types.hpp"

namespace EmperyCenter {

class ChatBox;
class HornMessage;

struct ChatBoxMap {
	static boost::shared_ptr<ChatBox> get(AccountUuid account_uuid);
	static boost::shared_ptr<ChatBox> require(AccountUuid account_uuid);
	static void unload(AccountUuid account_uuid);

	static boost::shared_ptr<HornMessage> get_horn_message(HornMessageUuid horn_message_uuid);
	static boost::shared_ptr<HornMessage> require_horn_message(HornMessageUuid horn_message_uuid);
	static boost::shared_ptr<HornMessage> get_or_reload_horn_message(HornMessageUuid horn_message_uuid);
	static boost::shared_ptr<HornMessage> forced_reload_horn_message(HornMessageUuid horn_message_uuid);

	static void get_all_horn_messages(std::vector<boost::shared_ptr<HornMessage>> &ret);
	static void get_horn_messages_by_language_id(std::vector<boost::shared_ptr<HornMessage>> &ret, LanguageId language_id);

	static void insert_horn_message(const boost::shared_ptr<HornMessage> &horn_message);
	static void update_horn_message(const boost::shared_ptr<HornMessage> &horn_message, bool throws_if_not_exists = true);

private:
	ChatBoxMap() = delete;
};

}

#endif
