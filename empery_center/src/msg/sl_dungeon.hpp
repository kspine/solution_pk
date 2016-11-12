#ifndef EMPERY_CENTER_MSG_SL_DUNGEON_HPP_
#define EMPERY_CENTER_MSG_SL_DUNGEON_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    SL_DungeonCreated
#define MESSAGE_ID      58601
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_VUINT         (dungeon_type_id)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SL_DungeonDeleted
#define MESSAGE_ID      58602
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_VUINT         (dungeon_type_id)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SL_DungeonFinish
#define MESSAGE_ID      58603
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_VUINT         (dungeon_type_id)	\
	FIELD_VUINT         (begin_time)	\
	FIELD_VUINT         (finish_time)	\
	FIELD_VUINT         (finished)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
