#ifndef EMPERY_CENTER_MSG_ST_ACCOUNT_HPP_
#define EMPERY_CENTER_MSG_ST_ACCOUNT_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    ST_AccountAddItems
#define MESSAGE_ID      20200
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_ARRAY         (items,	\
		FIELD_VUINT         (item_id)	\
		FIELD_VUINT         (count)	\
		FIELD_VUINT         (reason)	\
		FIELD_VINT          (param1)	\
		FIELD_VINT          (param2)	\
		FIELD_VINT          (param3)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    ST_AccountAcquireToken
#define MESSAGE_ID      20201
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    ST_AccountQueryToken
#define MESSAGE_ID      20202
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    ST_AccountInvalidate
#define MESSAGE_ID      20203
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
