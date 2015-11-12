#ifndef EMPERY_CENTER_MSG_SC_ACCOUNT_HPP_
#define EMPERY_CENTER_MSG_SC_ACCOUNT_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    SC_AccountLoginSuccess
#define MESSAGE_ID      299
#define MESSAGE_FIELDS  \
	FIELD_STRING        (accountUuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_AccountAttributes
#define MESSAGE_ID      298
#define MESSAGE_FIELDS  \
	FIELD_STRING        (accountUuid)	\
	FIELD_STRING        (nick)	\
	FIELD_ARRAY         (attributes,	\
		FIELD_VUINT         (slot)	\
		FIELD_STRING        (value)	\
	)	\
	FIELD_ARRAY         (publicItems,	\
		FIELD_VUINT         (itemId)	\
		FIELD_VUINT         (count)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_AccountQueryAttributesRet
#define MESSAGE_ID      297
#define MESSAGE_FIELDS  \
	FIELD_ARRAY         (accounts,	\
		FIELD_STRING        (accountUuid)	\
		FIELD_VUINT         (errorCode)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
