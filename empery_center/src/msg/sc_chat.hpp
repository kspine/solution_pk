#ifndef EMPERY_CENTER_MSG_SC_CHAT_HPP_
#define EMPERY_CENTER_MSG_SC_CHAT_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    SC_ChatMessageReceived
#define MESSAGE_ID      899
#define MESSAGE_FIELDS  \
	FIELD_STRING        (chat_message_uuid)	\
	FIELD_VUINT         (channel)	\
	FIELD_VUINT         (type)	\
	FIELD_VUINT         (language_id)	\
	FIELD_VUINT         (created_time)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_ChatMessageData
#define MESSAGE_ID      898
#define MESSAGE_FIELDS  \
	FIELD_STRING        (chat_message_uuid)	\
	FIELD_VUINT         (channel)	\
	FIELD_VUINT         (type)	\
	FIELD_VUINT         (language_id)	\
	FIELD_VUINT         (created_time)	\
	FIELD_STRING        (from_account_uuid)	\
	FIELD_ARRAY         (segments,	\
		FIELD_VUINT         (slot)	\
		FIELD_STRING        (value)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_ChatGetMessagesRet
#define MESSAGE_ID      897
#define MESSAGE_FIELDS  \
	FIELD_ARRAY         (chat_messages,  \
		FIELD_STRING        (chat_message_uuid)  \
		FIELD_VINT          (error_code)    \
	)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
