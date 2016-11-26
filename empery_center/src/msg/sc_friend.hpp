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
	FIELD_STRING        (metadata)	\
	FIELD_VUINT         (updated_time)	\
	FIELD_VUINT         (online)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_FriendRandomList
#define MESSAGE_ID      798
#define MESSAGE_FIELDS  \
	FIELD_ARRAY         (friends,	\
		FIELD_STRING        (friend_uuid)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_FriendPrivateMessage
#define MESSAGE_ID      797
#define MESSAGE_FIELDS  \
	FIELD_STRING        (friend_uuid)	\
	FIELD_VUINT         (language_id)	\
	FIELD_VUINT         (created_time)	\
	FIELD_ARRAY         (segments,	\
		FIELD_VUINT         (slot)	\
		FIELD_STRING        (value)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_FriendSearchByNickResult
#define MESSAGE_ID      796
#define MESSAGE_FIELDS  \
	FIELD_ARRAY         (results,	\
		FIELD_STRING        (other_uuid)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_FriendRecords
#define MESSAGE_ID      795
#define MESSAGE_FIELDS  \
	FIELD_ARRAY         (records,	\
		FIELD_STRING        (friend_uuid)	\
		FIELD_VUINT         (timestamp)     \
		FIELD_VINT          (result_type)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_FriendRecentContact
#define MESSAGE_ID      794
#define MESSAGE_FIELDS  \
	FIELD_ARRAY         (recentContact,	\
		FIELD_STRING        (friend_uuid)	\
		FIELD_VUINT         (timestamp)     \
		FIELD_VUINT         (online)        \
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_FriendOnlineStateChanged
#define MESSAGE_ID      793
#define MESSAGE_FIELDS  \
	FIELD_STRING        (friend_uuid)	\
	FIELD_VUINT         (online)	\
	FIELD_VUINT         (timestamp)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
