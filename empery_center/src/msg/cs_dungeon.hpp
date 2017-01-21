#ifndef EMPERY_CENTER_MSG_CS_DUNGEON_HPP_
#define EMPERY_CENTER_MSG_CS_DUNGEON_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    CS_DungeonGetAll
#define MESSAGE_ID      1500
#define MESSAGE_FIELDS  \
	//
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_DungeonCreate
#define MESSAGE_ID      1501
#define MESSAGE_FIELDS  \
	FIELD_VUINT         (dungeon_type_id)	\
	FIELD_ARRAY         (battalions,	\
		FIELD_STRING        (map_object_uuid)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_DungeonQuit
#define MESSAGE_ID      1502
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_DungeonSetWaypoints
#define MESSAGE_ID      1503
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_STRING        (dungeon_object_uuid)	\
	FIELD_ARRAY         (waypoints,	\
		FIELD_VINT          (dx)	\
		FIELD_VINT          (dy)	\
	)	\
	FIELD_VUINT         (action)	\
	FIELD_STRING		(param)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_DungeonStopTroops
#define MESSAGE_ID      1504
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_ARRAY         (dungeon_objects,	\
		FIELD_STRING        (dungeon_object_uuid)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_DungeonPlayerConfirmation
#define MESSAGE_ID      1505
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_STRING        (context)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_DungeonBegin
#define MESSAGE_ID      1506
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_ReconnDungeon
#define MESSAGE_ID      1507
#define MESSAGE_FIELDS  \
	//
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_ReconnResetScope
#define MESSAGE_ID      1508
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
