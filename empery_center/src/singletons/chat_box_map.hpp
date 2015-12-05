#ifndef EMPERY_CENTER_SINGLETONS_CHAT_BOX_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_CHAT_BOX_MAP_HPP_

#include <boost/shared_ptr.hpp>
#include "../id_types.hpp"

namespace EmperyCenter {

class ChatBox;

struct ChatBoxMap {
	static boost::shared_ptr<ChatBox> get(AccountUuid account_uuid);
	static boost::shared_ptr<ChatBox> require(AccountUuid account_uuid);

private:
	ChatBoxMap() = delete;
};

}

#endif
