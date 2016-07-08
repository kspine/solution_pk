#ifndef EMPERY_CENTER_MSG_ST_FRIEND_HPP_
#define EMPERY_CENTER_MSG_ST_FRIEND_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    ST_FriendPeerCompareExchange
#define MESSAGE_ID      20400
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_STRING        (transaction_uuid)	\
	FIELD_STRING        (friend_uuid)	\
	FIELD_ARRAY         (categories_expected,	\
		FIELD_VUINT         (category_expected)	\
	)	\
	FIELD_VUINT         (category)	\
	FIELD_VUINT         (max_count)	\
	FIELD_STRING        (metadata)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
