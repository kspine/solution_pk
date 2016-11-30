#ifndef EMPERY_CENTER_MSG_SC_TASK_HPP_
#define EMPERY_CENTER_MSG_SC_TASK_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    SC_TaskChanged
#define MESSAGE_ID      1299
#define MESSAGE_FIELDS  \
	FIELD_VUINT         (task_id)	\
	FIELD_VUINT         (category)	\
	FIELD_VUINT         (created_time)	\
	FIELD_VUINT         (expiry_duration)	\
	FIELD_ARRAY         (progress,	\
		FIELD_VUINT         (key)	\
		FIELD_VUINT         (count)	\
	)	\
	FIELD_VUINT         (rewarded)	\
	FIELD_VUINT         (finish_show)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_TaskLegionRewardChanged
#define MESSAGE_ID      1298
#define MESSAGE_FIELDS  \
	FIELD_VUINT         (task_type_id)	\
	FIELD_ARRAY         (progress,	\
		FIELD_VUINT         (key)	\
		FIELD_VUINT         (count)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
