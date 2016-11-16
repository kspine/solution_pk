#ifndef EMPERY_CENTER_MSG_SD_DUNGEON_HPP_
#define EMPERY_CENTER_MSG_SD_DUNGEON_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    SD_DungeonCreate
#define MESSAGE_ID      50099
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_VUINT         (dungeon_type_id)	\
	FIELD_STRING        (founder_uuid)	\
	FIELD_VUINT         (finish_count)	\
	FIELD_VUINT         (expiry_time)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SD_DungeonDestroy
#define MESSAGE_ID      50098
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SD_DungeonObjectInfo
#define MESSAGE_ID      50097
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_STRING        (dungeon_object_uuid)	\
	FIELD_VUINT         (map_object_type_id)	\
	FIELD_STRING        (owner_uuid)	\
	FIELD_VINT          (x)	\
	FIELD_VINT          (y)	\
	FIELD_ARRAY         (attributes,	\
		FIELD_VUINT          (attribute_id)	\
		FIELD_VINT           (value)	\
	)	\
	FIELD_ARRAY         (buffs,	\
		FIELD_VUINT          (buff_id)	\
		FIELD_VUINT          (time_begin)	\
		FIELD_VUINT          (time_end)	\
	)	\
	FIELD_STRING        (tag)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SD_DungeonObjectRemoved
#define MESSAGE_ID      50096
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_STRING        (dungeon_object_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SD_DungeonSetAction
#define MESSAGE_ID      50095
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_STRING        (dungeon_object_uuid)	\
	FIELD_VINT          (x)	\
	FIELD_VINT          (y)	\
	FIELD_ARRAY         (waypoints,	\
		FIELD_VINT          (dx)	\
		FIELD_VINT          (dy)	\
	)	\
	FIELD_VUINT         (action)	\
	FIELD_STRING        (param)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SD_DungeonPlayerConfirmation
#define MESSAGE_ID      50094
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_STRING        (context)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SD_DungeonBuffInfo
#define MESSAGE_ID      50093
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_VINT          (x)	\
	FIELD_VINT          (y)	\
	FIELD_VUINT         (buff_type_id)\
	FIELD_STRING        (create_uuid)\
	FIELD_STRING        (create_owner_uuid)\
	FIELD_VUINT         (expired_time)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SD_DungeonBuffRemoved
#define MESSAGE_ID      50092
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_VINT          (x)	\
	FIELD_VINT          (y)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SD_DungeonBegin
#define MESSAGE_ID      50091
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
