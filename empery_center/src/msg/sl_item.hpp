#ifndef EMPERY_CENTER_MSG_SL_ITEM_HPP_
#define EMPERY_CENTER_MSG_SL_ITEM_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    SL_ItemChanged
#define MESSAGE_ID      58201
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_VUINT         (item_id)	\
	FIELD_VUINT         (old_count)	\
	FIELD_VUINT         (new_count)	\
	FIELD_VUINT         (reason)	\
	FIELD_VINT          (param1)	\
	FIELD_VINT          (param2)	\
	FIELD_VINT          (param3)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
