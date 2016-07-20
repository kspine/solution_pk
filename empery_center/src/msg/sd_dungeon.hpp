#ifndef EMPERY_CENTER_MSG_SD_DUNGEON_HPP_
#define EMPERY_CENTER_MSG_SD_DUNGEON_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    SD_DungeonCreate
#define MESSAGE_ID      50099
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_VUINT         (dungeon_type_id)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SD_DungeonDestroy
#define MESSAGE_ID      50098
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SD_DungeonInsertObject
#define MESSAGE_ID      50097
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_STRING        (dungeon_object_uuid)	\
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
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SD_DungeonRemoveObject
#define MESSAGE_ID      50096
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_STRING        (dungeon_object_uuid)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
