#ifndef EMPERY_CENTER_MSG_TS_ACCOUNT_HPP_
#define EMPERY_CENTER_MSG_TS_ACCOUNT_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    TS_AccountSendPromotionBonus
#define MESSAGE_ID      20299
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_STRING        (taxer_uuid)	\
	FIELD_VUINT         (amount)	\
	FIELD_VUINT         (reason)	\
	FIELD_VINT          (param1)	\
	FIELD_VINT          (param2)	\
	FIELD_VINT          (param3)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    TS_AccountInvalidate
#define MESSAGE_ID      20298
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_VINT          (reason)	\
	FIELD_STRING        (param)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
