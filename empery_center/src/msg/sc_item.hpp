#ifndef EMPERY_CENTER_MSG_SC_ITEM_HPP_
#define EMPERY_CENTER_MSG_SC_ITEM_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    SC_ItemChanged
#define MESSAGE_ID      599
#define MESSAGE_FIELDS  \
	FIELD_VUINT         (item_id)	\
	FIELD_VUINT         (count)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
