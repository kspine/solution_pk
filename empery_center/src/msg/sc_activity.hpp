#ifndef EMPERY_CENTER_MSG_SC_ITEM_HPP_
#define EMPERY_CENTER_MSG_SC_ITEM_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    SC_MapActivityInfo
#define MESSAGE_ID      1499
#define MESSAGE_FIELDS  \
	FIELD_ARRAY         (activitys,	\
		FIELD_VUINT         (unique_id)	\
		FIELD_VUINT         (available_since)	\
		FIELD_VUINT         (available_until)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
