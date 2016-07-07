#ifndef EMPERY_CENTER_MSG_SC_FRIEND_HPP_
#define EMPERY_CENTER_MSG_SC_FRIEND_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    SC_FriendChanged
#define MESSAGE_ID      799
#define MESSAGE_FIELDS  \
	FIELD_STRING        (friend_uuid)	\
	FIELD_VUINT         (category)	\
	FIELD_STRING        (metadata)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_FriendRandomList
#define MESSAGE_ID      798
#define MESSAGE_FIELDS  \
	FIELD_ARRAY         (friends,	\
		FIELD_STRING        (friend_uuid)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
