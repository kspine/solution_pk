#ifndef EMPERY_CENTER_MSG_SL_MAP_HPP_
#define EMPERY_CENTER_MSG_SL_MAP_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    SL_MapEventsOverflowed
#define MESSAGE_ID      58501
#define MESSAGE_FIELDS  \
	FIELD_VINT          (block_x)	\
	FIELD_VINT          (block_y)	\
	FIELD_VUINT         (width)	\
	FIELD_VUINT         (height)	\
	FIELD_VUINT         (active_castle_count)	\
	FIELD_VUINT         (map_event_id)	\
	FIELD_VUINT         (events_to_refresh)	\
	FIELD_VUINT         (events_retained)	\
	FIELD_VUINT         (events_created)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
