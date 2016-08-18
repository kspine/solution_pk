#ifndef EMPERY_CENTER_MSG_DS_DUNGEON_HPP_
#define EMPERY_CENTER_MSG_DS_DUNGEON_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    DS_DungeonRegisterServer
#define MESSAGE_ID      50000
#define MESSAGE_FIELDS  \
	//
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    DS_DungeonUpdateObjectAction
#define MESSAGE_ID      50001
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_STRING        (dungeon_object_uuid)	\
	FIELD_VINT          (x)	\
	FIELD_VINT          (y)	\
	FIELD_VUINT         (action)	\
	FIELD_STRING        (param)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    DS_DungeonObjectAttackAction
#define MESSAGE_ID      50002
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_STRING        (attacking_account_uuid)	\
	FIELD_STRING        (attacking_object_uuid)	\
	FIELD_VUINT         (attacking_object_type_id)	\
	FIELD_VINT          (attacking_coord_x)	\
	FIELD_VINT          (attacking_coord_y)	\
	FIELD_STRING        (attacked_account_uuid)	\
	FIELD_STRING        (attacked_object_uuid)	\
	FIELD_VUINT         (attacked_object_type_id)	\
	FIELD_VINT          (attacked_coord_x)	\
	FIELD_VINT          (attacked_coord_y)	\
	FIELD_VINT          (result_type)	\
	FIELD_VINT          (result_param1)	\
	FIELD_VINT          (result_param2)	\
	FIELD_VUINT         (soldiers_damaged)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    DS_DungeonSetScope
#define MESSAGE_ID      50003
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_VINT          (x)	\
	FIELD_VINT          (y)	\
	FIELD_VUINT         (width)	\
	FIELD_VUINT         (height)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    DS_DungeonCreateMonster
#define MESSAGE_ID      50004
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_VUINT         (map_object_type_id)	\
	FIELD_VINT          (x)	\
	FIELD_VINT          (y)	\
	FIELD_VINT          (dest_x)	\
	FIELD_VINT          (dest_y)	\
	FIELD_STRING        (tag)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    DS_DungeonWaitForPlayerConfirmation
#define MESSAGE_ID      50005
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_STRING        (context)	\
	FIELD_VUINT         (type)	\
	FIELD_VINT          (param1)	\
	FIELD_VINT          (param2)	\
	FIELD_STRING        (param3)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    DS_DungeonObjectStopped
#define MESSAGE_ID      50006
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_STRING        (dungeon_object_uuid)   \
	FIELD_VUINT         (action)    \
	FIELD_STRING        (param) \
	FIELD_VINT          (error_code)    \
	FIELD_STRING        (error_message)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    DS_DungeonWaypointsSet
#define MESSAGE_ID      50007
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_STRING        (dungeon_object_uuid)   \
	FIELD_VINT          (x)	\
	FIELD_VINT          (y)	\
	FIELD_ARRAY         (waypoints,	\
		FIELD_VINT          (dx)	\
		FIELD_VINT          (dy)	\
	)	\
	FIELD_VUINT         (action)	\
	FIELD_STRING        (param)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    DS_DungeonPlayerWins
#define MESSAGE_ID      50008
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_STRING        (account_uuid)	\
	FIELD_ARRAY         (tasks_finished,	\
		FIELD_VUINT         (dungeon_task_id)	\
	)	\
	FIELD_ARRAY         (damage_solider,	\
		FIELD_VUINT        (dungeon_object_type_id)	\
		FIELD_VUINT        (count)	\
	)	\
	FIELD_VUINT         (total_damage_solider)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    DS_DungeonPlayerLoses
#define MESSAGE_ID      50009
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_STRING        (account_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    DS_DungeonMoveCamera
#define MESSAGE_ID      50010
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)  \
	FIELD_VINT          (x) \
	FIELD_VINT          (y) \
	FIELD_VUINT         (movement_duration)	\
	FIELD_VUINT         (position_type)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
