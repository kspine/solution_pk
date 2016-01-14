#ifndef EMPERY_CENTER_MSG_SC_ACCOUNT_HPP_
#define EMPERY_CENTER_MSG_SC_ACCOUNT_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    SC_AccountLoginSuccess
#define MESSAGE_ID      299
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_AccountAttributes
#define MESSAGE_ID      298
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_STRING        (nick)	\
	FIELD_ARRAY         (attributes,	\
		FIELD_VUINT         (account_attribute_id)	\
		FIELD_STRING        (value)	\
	)	\
	FIELD_ARRAY         (public_items,	\
		FIELD_VUINT         (item_id)	\
		FIELD_VUINT         (item_count)	\
	)	\
	FIELD_VUINT         (promotion_level)	\
	FIELD_VUINT         (activated)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_AccountQueryAttributesRet
#define MESSAGE_ID      297
#define MESSAGE_FIELDS  \
	FIELD_ARRAY         (accounts,	\
		FIELD_STRING        (account_uuid)	\
		FIELD_VINT          (error_code)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_AccountSynchronizeSystemClock
#define MESSAGE_ID      296
#define MESSAGE_FIELDS  \
	FIELD_STRING        (context)	\
	FIELD_VUINT         (timestamp)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
