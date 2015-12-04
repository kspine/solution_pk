#ifndef EMPERY_CENTER_MSG_CS_ANNOUNCEMENT_HPP_
#define EMPERY_CENTER_MSG_CS_ANNOUNCEMENT_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    CS_AnnouncementGetAnnouncements
#define MESSAGE_ID      900
#define MESSAGE_FIELDS  \
	FIELD_VUINT         (language_id)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
