#ifndef EMPERY_CENTER_MSG_SC_ACTIVITY_HPP_
#define EMPERY_CENTER_MSG_SC_ACTIVITY_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    SC_MapActivityInfo
#define MESSAGE_ID      1499
#define MESSAGE_FIELDS  \
	FIELD_VUINT         (curr_unique_id)	\
	FIELD_ARRAY         (activitys,	\
		FIELD_VUINT         (unique_id)	\
		FIELD_VUINT         (available_since)	\
		FIELD_VUINT         (available_until)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_MapActivityKillSolidersRank
#define MESSAGE_ID      1498
#define MESSAGE_FIELDS  \
	FIELD_ARRAY         (rank_list,	\
		FIELD_STRING        (account_uuid)      \
		FIELD_STRING        (nick)	            \
		FIELD_STRING        (castle_name)	    \
		FIELD_STRING        (leagues)	        \
		FIELD_VUINT         (rank)	            \
		FIELD_VUINT         (accumulate_value)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
