#ifndef EMPERY_CENTER_MSG_CS_CHAT_HPP_
#define EMPERY_CENTER_MSG_CS_CHAT_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    CS_ChatSendMessage
#define MESSAGE_ID      800
#define MESSAGE_FIELDS  \
	FIELD_VUINT         (channel)	\
	FIELD_VUINT         (type)	\
	FIELD_VUINT         (language_id)	\
	FIELD_ARRAY         (segments,	\
		FIELD_VUINT         (slot)	\
		FIELD_STRING        (value)	\
	)	\
	FIELD_VINT          (x)	\
	FIELD_VINT          (y)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_ChatGetMessages
#define MESSAGE_ID      801
#define MESSAGE_FIELDS  \
	FIELD_VUINT         (language_id)	\
	FIELD_ARRAY         (chat_messages,	\
		FIELD_STRING        (chat_message_uuid)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
