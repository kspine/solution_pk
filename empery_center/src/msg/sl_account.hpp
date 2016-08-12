#ifndef EMPERY_CENTER_MSG_SL_ACCOUNT_HPP_
#define EMPERY_CENTER_MSG_SL_ACCOUNT_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    SL_AccountCreated
#define MESSAGE_ID      58101
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_STRING        (remote_ip)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SL_AccountLoggedIn
#define MESSAGE_ID      58102
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_STRING        (remote_ip)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SL_AccountLoggedOut
#define MESSAGE_ID      58103
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_VUINT         (online_duration)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SL_AccountNumberOnline
#define MESSAGE_ID      58105
#define MESSAGE_FIELDS  \
	FIELD_VUINT         (interval)	\
	FIELD_VUINT         (account_count)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
