#ifndef EMPERY_CENTER_MYSQL_ACCOUNT_HPP_
#define EMPERY_CENTER_MYSQL_ACCOUNT_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenter {

namespace MySql {

#define MYSQL_OBJECT_NAME   Center_Account
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (account_uuid)	\
	FIELD_INTEGER_UNSIGNED  (platform_id)	\
	FIELD_STRING            (login_name)	\
	FIELD_UUID              (referrer_uuid)	\
	FIELD_INTEGER_UNSIGNED  (promotion_level)	\
	FIELD_DATETIME          (created_time)	\
	FIELD_STRING            (nick)	\
	FIELD_BOOLEAN           (activated)	\
	FIELD_DATETIME          (banned_until)	\
	FIELD_DATETIME          (quieted_until)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   Center_AccountAttribute
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (account_uuid)	\
	FIELD_INTEGER_UNSIGNED  (account_attribute_id)	\
	FIELD_STRING            (value)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
