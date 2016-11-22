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

#define MESSAGE_NAME    CS_FriendRandom
#define MESSAGE_ID      706
#define MESSAGE_FIELDS  \
	FIELD_VUINT         (max_count)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_FriendPrivateMessage
#define MESSAGE_ID      707
#define MESSAGE_FIELDS  \
	FIELD_STRING        (friend_uuid)	\
	FIELD_VUINT         (language_id)	\
	FIELD_ARRAY         (segments,	\
		FIELD_VUINT         (slot)	\
		FIELD_STRING        (value)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_FriendSearchByNick
#define MESSAGE_ID      708
#define MESSAGE_FIELDS  \
	FIELD_STRING        (nick)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_FriendBlackListAdd
#define MESSAGE_ID      709
#define MESSAGE_FIELDS  \
	FIELD_STRING        (friend_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_FriendBlackListDelete
#define MESSAGE_ID      710
#define MESSAGE_FIELDS  \
	FIELD_STRING        (friend_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_FriendRecords
#define MESSAGE_ID      711
#define MESSAGE_FIELDS  \
	//
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_FriendGetOfflineMsg
#define MESSAGE_ID      712
#define MESSAGE_FIELDS  \
	//
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_FriendGetRecent
#define MESSAGE_ID      713
#define MESSAGE_FIELDS  \
	//
#include <poseidon/cbpp/message_generator.hpp>


}

}

#endif
