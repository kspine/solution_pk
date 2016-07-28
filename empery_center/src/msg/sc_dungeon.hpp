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

#define MESSAGE_NAME    SC_DungeonObjectStopped
#define MESSAGE_ID      1594
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_STRING        (dungeon_object_uuid)	\
	FIELD_VUINT         (action)	\
	FIELD_STRING        (param)	\
	FIELD_VINT          (error_code)	\
	FIELD_STRING        (error_message)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_DungeonStopTroopsRet
#define MESSAGE_ID      1593
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_ARRAY         (dungeon_objects,   \
		FIELD_STRING        (dungeon_object_uuid)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_DungeonWaypointsSet
#define MESSAGE_ID      1592
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

#define MESSAGE_NAME    SC_DungeonObjectAttackResult
#define MESSAGE_ID      1591
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_STRING        (attacking_object_uuid)	\
	FIELD_VINT          (attacking_coord_x)	\
	FIELD_VINT          (attacking_coord_y)	\
	FIELD_STRING        (attacked_object_uuid)	\
	FIELD_VINT          (attacked_coord_x)	\
	FIELD_VINT          (attacked_coord_y)	\
	FIELD_VINT          (result_type)	\
	FIELD_VUINT         (soldiers_resuscitated)	\
	FIELD_VUINT         (soldiers_wounded)	\
	FIELD_VUINT         (soldiers_wounded_added)	\
	FIELD_VUINT         (soldiers_damaged)	\
	FIELD_VUINT         (soldiers_remaining)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_DungeonMonsterRewardGot
#define MESSAGE_ID      1590
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_VINT          (x)	\
	FIELD_VINT          (y)	\
	FIELD_VUINT         (map_object_type_id)	\
	FIELD_ARRAY         (items_basic,	\
		FIELD_VUINT         (item_id)	\
		FIELD_VUINT         (count)	\
	)	\
	FIELD_STRING        (castle_uuid)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
