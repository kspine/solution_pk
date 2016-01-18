#ifndef EMPERY_CENTER_MSG_CS_MAIL_HPP_
#define EMPERY_CENTER_MSG_CS_MAIL_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    CS_MailGetAllMails
#define MESSAGE_ID      600
#define MESSAGE_FIELDS  \
	//
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_MailGetMailData
#define MESSAGE_ID      601
#define MESSAGE_FIELDS  \
	FIELD_ARRAY         (mails,	\
		FIELD_STRING        (mail_uuid)	\
		FIELD_VUINT         (language_id)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_MailWriteToAccount
#define MESSAGE_ID      602
#define MESSAGE_FIELDS  \
	FIELD_STRING        (to_account_uuid)	\
	FIELD_VUINT         (type)	\
	FIELD_STRING        (subject)	\
	FIELD_ARRAY         (segments,	\
		FIELD_VUINT         (slot)	\
		FIELD_STRING        (value)	\
	)	\
	FIELD_VUINT         (language_id)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_MailMarkAsRead
#define MESSAGE_ID      603
#define MESSAGE_FIELDS  \
	FIELD_STRING        (mail_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_MailFetchAttachments
#define MESSAGE_ID      604
#define MESSAGE_FIELDS  \
	FIELD_STRING        (mail_uuid)	\
	FIELD_VUINT         (language_id)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_MailDelete
#define MESSAGE_ID      605
#define MESSAGE_FIELDS  \
	FIELD_STRING        (mail_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_MailBatchReadAndFetchAttachments
#define MESSAGE_ID      606
#define MESSAGE_FIELDS  \
	FIELD_VUINT         (language_id)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_MailBatchDelete
#define MESSAGE_ID      607
#define MESSAGE_FIELDS  \
	//
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
