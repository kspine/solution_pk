#ifndef EMPERY_CENTER_MSG_SC_MAIL_HPP_
#define EMPERY_CENTER_MSG_SC_MAIL_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    SC_MailChanged
#define MESSAGE_ID      699
#define MESSAGE_FIELDS  \
	FIELD_STRING        (mail_uuid)	\
	FIELD_VUINT         (expiry_duration)	\
	FIELD_VUINT         (flags)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_MailText
#define MESSAGE_ID      698
#define MESSAGE_FIELDS  \
	FIELD_STRING        (mail_uuid)	\
	FIELD_VUINT         (language_id)	\
	FIELD_VUINT         (created_time)	\
	FIELD_VUINT         (type)	\
	FIELD_VUINT         (from_account_uuid)	\
	FIELD_STRING        (summary)	\
	FIELD_STRING        (body)	\
	FIELD_ARRAY         (attachments,	\
		FIELD_VUINT         (item_id)	\
		FIELD_VUINT         (item_count)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_MailNewMailReceived
#define MESSAGE_ID      697
#define MESSAGE_FIELDS  \
	FIELD_STRING        (mail_uuid)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
