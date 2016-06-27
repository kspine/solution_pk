#ifndef EMPERY_CENTER_MSG_CS_DUNGEON_HPP_
#define EMPERY_CENTER_MSG_CS_DUNGEON_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    CS_DungeonGetRecords
#define MESSAGE_ID      1500
#define MESSAGE_FIELDS  \
	//
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_DungeonGetRecords
#define MESSAGE_ID      1501
#define MESSAGE_FIELDS  \
	FIELD_VUINT         (dungeon_id)	\
	FIELD_ARRAY         (battalions,	\
		FIELD_STRING        (map_object_uuid)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
