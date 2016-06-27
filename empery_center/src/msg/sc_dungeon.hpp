#ifndef EMPERY_CENTER_MSG_SC_DUNGEON_HPP_
#define EMPERY_CENTER_MSG_SC_DUNGEON_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    SC_DungeonRecords
#define MESSAGE_ID      1599
#define MESSAGE_FIELDS  \
	FIELD_ARRAY         (dungeons,	\
		FIELD_VUINT         (dungeon_id)	\
		FIELD_VUINT         (score)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
