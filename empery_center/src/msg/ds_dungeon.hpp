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

}

}

#endif
