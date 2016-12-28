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
	FIELD_STRING        (param)	\
	FIELD_VINT          (target_x)	\
	FIELD_VINT          (target_y)
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

#define MESSAGE_NAME    DS_DungeonTriggerEffectForcast
#define MESSAGE_ID      50011
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_VUINT         (trigger_id) \
	FIELD_VUINT         (executive_time) \
	FIELD_ARRAY         (effects,	\
		FIELD_VUINT        (effect_type)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    DS_DungeonTriggerEffectExecutive
#define MESSAGE_ID      50012
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_VUINT         (trigger_id) \
	FIELD_ARRAY         (effects,	\
		FIELD_VUINT        (effect_type)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    DS_DungeonShowPicture
#define MESSAGE_ID      50013
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_STRING        (picture_url)	\
	FIELD_VUINT         (picture_id)	\
	FIELD_VINT          (type)	\
	FIELD_VINT          (layer)	\
	FIELD_VINT          (tween)	\
	FIELD_VINT          (time)	\
	FIELD_VINT          (x)	\
	FIELD_VINT          (y)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    DS_DungeonRemovePicture
#define MESSAGE_ID      50014
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_VUINT         (picture_id)	\
	FIELD_VINT          (tween)	\
	FIELD_VINT          (time)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    DS_DungeonCreateBuff
#define MESSAGE_ID      50015
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_VUINT         (buff_type_id)	\
	FIELD_VINT          (x)	\
	FIELD_VINT          (y)	\
	FIELD_STRING        (create_uuid) \
	FIELD_STRING        (create_owner_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    DS_DungeonCreateBlocks
#define MESSAGE_ID      50016
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_ARRAY         (blocks,	\
		FIELD_VINT        (x)	\
		FIELD_VINT        (y)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    DS_DungeonRemoveBlocks
#define MESSAGE_ID      50017
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_ARRAY         (blocks,	\
		FIELD_VINT        (x)	\
		FIELD_VINT        (y)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    DS_DungeonHideSolider
#define MESSAGE_ID      50018
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid) \
	FIELD_VUINT         (type)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    DS_DungeonUnhideSolider
#define MESSAGE_ID      50019
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid) \
	FIELD_VUINT         (type)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    DS_DungeonHideCoords
#define MESSAGE_ID      50020
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid) \
	FIELD_ARRAY         (hide_coord,	\
		FIELD_VINT        (x)	\
		FIELD_VINT        (y)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    DS_DungeonUnhideCoords
#define MESSAGE_ID      50021
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid) \
	FIELD_ARRAY         (unhide_coord,	\
		FIELD_VINT        (x)	\
		FIELD_VINT        (y)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    DS_DungeonObjectSkillSingAction
#define MESSAGE_ID      50022
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_STRING        (attacking_account_uuid)	\
	FIELD_STRING        (attacking_object_uuid)	\
	FIELD_VUINT         (attacking_object_type_id)	\
	FIELD_VINT          (attacking_coord_x)	\
	FIELD_VINT          (attacking_coord_y)	\
	FIELD_VINT          (attacked_coord_x)	\
	FIELD_VINT          (attacked_coord_y)	\
	FIELD_VUINT         (skill_type_id)	\
	FIELD_VUINT          (sing_delay)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    DS_DungeonObjectSkillCastAction
#define MESSAGE_ID      50023
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_STRING        (attacking_account_uuid)	\
	FIELD_STRING        (attacking_object_uuid)	\
	FIELD_VUINT         (attacking_object_type_id)	\
	FIELD_VINT          (attacking_coord_x)	\
	FIELD_VINT          (attacking_coord_y)	\
	FIELD_VINT          (attacked_coord_x)	\
	FIELD_VINT          (attacked_coord_y)	\
	FIELD_VUINT         (skill_type_id)	\
	FIELD_VUINT          (cast_delay)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    DS_DungeonObjectSkillEffect
#define MESSAGE_ID      50024
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_STRING        (attacking_account_uuid)	\
	FIELD_STRING        (attacking_object_uuid)	\
	FIELD_VUINT         (attacking_object_type_id)	\
	FIELD_VINT          (attacking_coord_x)	\
	FIELD_VINT          (attacking_coord_y)	\
	FIELD_VINT          (attacked_coord_x)	\
	FIELD_VINT          (attacked_coord_y)	\
	FIELD_VUINT         (skill_type_id)	\
	FIELD_ARRAY         (effect_range,	\
		FIELD_VINT        (x)	\
		FIELD_VINT        (y)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    DS_DungeonObjectPrepareTransmit
#define MESSAGE_ID      50025
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_ARRAY         (transmit_objects,	\
		FIELD_STRING      (object_uuid)	\
		FIELD_VINT        (x)	\
		FIELD_VINT        (y)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    DS_DungeonObjectAddBuff
#define MESSAGE_ID      50026
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_STRING        (dungeon_object_uuid)	\
	FIELD_VUINT         (buff_type_id)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    DS_DungeonSetFootAnnimation
#define MESSAGE_ID      50027
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_STRING        (picture_url)	\
	FIELD_VINT          (type)	\
	FIELD_VINT          (x)	\
	FIELD_VINT          (y)	\
	FIELD_VINT          (layer)	\
	FIELD_ARRAY         (monsters,	\
		FIELD_STRING      (monster_uuid)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    DS_DungeonObjectClearBuff
#define MESSAGE_ID      50028
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_STRING        (dungeon_object_uuid)	\
	FIELD_VUINT         (buff_type_id)
#include <poseidon/cbpp/message_generator.hpp>
}

}

#endif
