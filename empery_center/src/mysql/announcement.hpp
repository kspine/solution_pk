#ifndef EMPERY_CENTER_MYSQL_ANNOUNCEMENT_HPP_
#define EMPERY_CENTER_MYSQL_ANNOUNCEMENT_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenter {

namespace MySql {

#define MYSQL_OBJECT_NAME   Center_Announcement
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (announcement_uuid)  \
	FIELD_INTEGER_UNSIGNED  (language_id)   \
	FIELD_DATETIME          (created_time)	\
	FIELD_DATETIME          (expiry_time)	\
	FIELD_BIGINT_UNSIGNED   (period)	\
	FIELD_INTEGER_UNSIGNED  (type)	\
	FIELD_STRING            (segments)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
