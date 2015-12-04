#ifndef EMPERY_CENTER_MSG_SC_ANNOUNCEMENT_HPP_
#define EMPERY_CENTER_MSG_SC_ANNOUNCEMENT_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    SC_AnnouncementReceived
#define MESSAGE_ID      999
#define MESSAGE_FIELDS  \
	FIELD_STRING        (announcement_uuid)	\
	FIELD_VUINT         (language_id)	\
	FIELD_VUINT         (created_time)	\
	FIELD_VUINT         (expiry_time)	\
	FIELD_VUINT         (period)	\
	FIELD_ARRAY         (segments,	\
		FIELD_VUINT         (slot)	\
		FIELD_STRING        (value)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
