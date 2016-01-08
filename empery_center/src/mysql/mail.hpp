#ifndef EMPERY_CENTER_MYSQL_MAIL_HPP_
#define EMPERY_CENTER_MYSQL_MAIL_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenter {

namespace MySql {

#define MYSQL_OBJECT_NAME   Center_Mail
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (mail_uuid)	\
	FIELD_UUID              (account_uuid)	\
	FIELD_DATETIME          (expiry_time)	\
	FIELD_TINYINT_UNSIGNED  (system)	\
	FIELD_TINYINT_UNSIGNED  (read)	\
	FIELD_TINYINT_UNSIGNED  (attachments_fetched)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   Center_MailData
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (mail_uuid)	\
	FIELD_INTEGER_UNSIGNED  (language_id)	\
	FIELD_DATETIME          (created_time)	\
	FIELD_INTEGER_UNSIGNED  (type)	\
	FIELD_UUID              (from_account_uuid)	\
	FIELD_STRING            (subject)	\
	FIELD_STRING            (segments)	\
	FIELD_STRING            (attachments)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
