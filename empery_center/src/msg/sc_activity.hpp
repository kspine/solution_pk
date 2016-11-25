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


#define MESSAGE_NAME    SC_WorldActivityInfo
#define MESSAGE_ID      1497
#define MESSAGE_FIELDS  \
	FIELD_VUINT         (curr_activity_id)	\
	FIELD_VUINT         (since)	\
	FIELD_VUINT         (until)	\
	FIELD_ARRAY         (activitys,	\
		FIELD_VUINT         (unique_id)	\
		FIELD_VUINT         (sub_since)	\
		FIELD_VUINT         (sub_until)	\
		FIELD_VUINT         (objective)	\
		FIELD_VUINT         (schedule)	\
		FIELD_VUINT         (contribute)\
		FIELD_VUINT         (finish)\
		FIELD_VUINT         (rewarded) \
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_WorldActivityRank
#define MESSAGE_ID      1496
#define MESSAGE_FIELDS  \
	FIELD_VUINT         (since)	\
	FIELD_VUINT         (rank)  \
	FIELD_VUINT         (accumulate_value)  \
	FIELD_VUINT         (rewarded) \
	FIELD_ARRAY         (rank_list,	\
		FIELD_STRING        (account_uuid)      \
		FIELD_STRING        (nick)	            \
		FIELD_STRING        (castle_name)	    \
		FIELD_STRING        (leagues)	        \
		FIELD_VUINT         (rank)	            \
		FIELD_VUINT         (accumulate_value)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_WorldBossPos
#define MESSAGE_ID      1495
#define MESSAGE_FIELDS  \
	FIELD_VINT          (x)	\
	FIELD_VINT          (y)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_WorldActivityDropData
#define MESSAGE_ID      1494
#define MESSAGE_FIELDS  \
	FIELD_VUINT         (unique_id) \
	FIELD_ARRAY         (drop,	\
		FIELD_STRING        (collection_name)      \
		FIELD_VUINT         (count)	            \
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_ActivityRankAwardData
#define MESSAGE_ID      1493
#define MESSAGE_FIELDS  \
	FIELD_ARRAY         (rank_list,	\
		FIELD_VUINT         (begin)	\
		FIELD_VUINT         (end)	\
		FIELD_ARRAY         (reward,	\
			FIELD_VUINT         (item_id) \
			FIELD_VUINT         (count)	\
		)\
	)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
