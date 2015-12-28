#ifndef EMPERY_CENTER_MSG_SC_PACKED_HPP_
#define EMPERY_CENTER_MSG_SC_PACKED_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    SC_PackedResponse
#define MESSAGE_ID      69
#define MESSAGE_FIELDS  \
	FIELD_ARRAY         (messages,	\
		FIELD_VUINT         (message_id)    \
		FIELD_STRING        (payload)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
