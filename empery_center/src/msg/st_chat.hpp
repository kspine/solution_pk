#ifndef EMPERY_CENTER_MSG_ST_CHAT_HPP_
#define EMPERY_CENTER_MSG_ST_CHAT_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    ST_ChatInvalidateHornMessage
#define MESSAGE_ID      20500
#define MESSAGE_FIELDS  \
	FIELD_STRING        (horn_message_uuid)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
