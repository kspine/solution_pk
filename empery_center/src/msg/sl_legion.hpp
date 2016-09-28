#ifndef EMPERY_CENTER_MSG_SL_LEGION_HPP_
#define EMPERY_CENTER_MSG_SL_LEGION_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    SL_LegionDisbandLog
#define MESSAGE_ID      58701
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_STRING        (legion_name)	\
	FIELD_VUINT         (disband_time)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SL_LeagueDisbandLog
#define MESSAGE_ID      58702
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_STRING        (league_name)	\
	FIELD_VUINT         (disband_time)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
