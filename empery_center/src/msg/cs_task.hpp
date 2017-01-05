#ifndef EMPERY_CENTER_MSG_CS_TASK_HPP_
#define EMPERY_CENTER_MSG_CS_TASK_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    CS_ItemGetAllTasks
#define MESSAGE_ID      1200
#define MESSAGE_FIELDS  \
	//
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_ItemFetchTaskReward
#define MESSAGE_ID      1201
#define MESSAGE_FIELDS  \
	FIELD_VUINT         (task_id)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_ItemFetchLegionTaskReward
#define MESSAGE_ID      1202
#define MESSAGE_FIELDS  \
	FIELD_STRING         (legion_uuid) \
	FIELD_VUINT          (task_id)	\
	FIELD_VUINT          (stage)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
