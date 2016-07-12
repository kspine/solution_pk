#ifndef EMPERY_CENTER_MYSQL_HORN_MESSAGE_HPP_
#define EMPERY_CENTER_MYSQL_HORN_MESSAGE_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenter {

namespace MySql {

#define MYSQL_OBJECT_NAME   Center_HornMessage
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (horn_message_uuid)	\
	FIELD_INTEGER_UNSIGNED  (item_id)	\
	FIELD_INTEGER_UNSIGNED  (language_id)	\
	FIELD_DATETIME          (created_time)	\
	FIELD_DATETIME          (expiry_time)	\
	FIELD_UUID              (from_account_uuid)	\
	FIELD_STRING            (segments)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
