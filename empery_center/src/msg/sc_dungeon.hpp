#ifndef EMPERY_CENTER_MSG_SC_DUNGEON_HPP_
#define EMPERY_CENTER_MSG_SC_DUNGEON_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    SC_DungeonScoreChanged
#define MESSAGE_ID      1599
#define MESSAGE_FIELDS  \
	FIELD_VUINT         (dungeon_type_id)	\
	FIELD_VUINT         (score)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_DungeonEntered
#define MESSAGE_ID      1598
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_VUINT         (dungeon_type_id)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_DungeonLeft
#define MESSAGE_ID      1597
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_VINT          (reason)	\
	FIELD_STRING        (param)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_DungeonObjectInfo
#define MESSAGE_ID      1596
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_STRING        (dungeon_object_uuid)	\
	FIELD_VUINT         (map_object_type_id)	\
	FIELD_STRING        (owner_uuid)	\
	FIELD_VINT          (x)	\
	FIELD_VINT          (y)	\
	FIELD_ARRAY         (attributes,	\
		FIELD_VUINT         (attribute_id)	\
		FIELD_VINT          (value)	\
	)	\
	FIELD_VUINT         (action)	\
	FIELD_STRING        (param)	\
	FIELD_ARRAY         (buffs,	\
		FIELD_VUINT         (buff_id)	\
		FIELD_VUINT         (duration)	\
		FIELD_VUINT         (time_remaining)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_DungeonObjectRemoved
#define MESSAGE_ID      1595
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_STRING        (dungeon_object_uuid)	\
	FIELD_VUINT         (map_object_type_id)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
