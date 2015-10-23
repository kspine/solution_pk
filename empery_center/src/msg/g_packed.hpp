#ifndef EMPERY_CENTER_MSG_G_PACKED_REQUEST_HPP_
#define EMPERY_CENTER_MSG_G_PACKED_REQUEST_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME	G_PackedRequest
#define MESSAGE_ID		99
#define MESSAGE_FIELDS	\
	FIELD_VUINT			(serial)	\
	FIELD_VUINT			(messageId)	\
	FIELD_STRING		(payload)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME	G_PackedResponse
#define MESSAGE_ID		98
#define MESSAGE_FIELDS	\
	FIELD_VUINT			(serial)	\
	FIELD_VINT			(code)	\
	FIELD_STRING		(message)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
