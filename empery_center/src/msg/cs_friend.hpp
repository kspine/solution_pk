#ifndef EMPERY_CENTER_MSG_CS_FRIEND_HPP_
#define EMPERY_CENTER_MSG_CS_FRIEND_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    CS_FriendGetAll
#define MESSAGE_ID      700
#define MESSAGE_FIELDS  \
	//
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_FriendRequest
#define MESSAGE_ID      701
#define MESSAGE_FIELDS  \
	FIELD_STRING        (friend_uuid)	\
	FIELD_STRING        (metadata)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_FriendAccept
#define MESSAGE_ID      702
#define MESSAGE_FIELDS  \
	FIELD_STRING        (friend_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_FriendDecline
#define MESSAGE_ID      703
#define MESSAGE_FIELDS  \
	FIELD_STRING        (friend_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_FriendDelete
#define MESSAGE_ID      704
#define MESSAGE_FIELDS  \
	FIELD_STRING        (friend_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_FriendCancelRequest
#define MESSAGE_ID      705
#define MESSAGE_FIELDS  \
	FIELD_STRING        (friend_uuid)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
