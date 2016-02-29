#ifndef EMPERY_CENTER_MYSQL_MAP_EVENT_HPP_
#define EMPERY_CENTER_MYSQL_MAP_EVENT_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenter {

namespace MySql {

#define MYSQL_OBJECT_NAME   Center_MapEvent
#define MYSQL_OBJECT_FIELDS \
	FIELD_BIGINT            (x)	\
	FIELD_BIGINT            (y)	\
	FIELD_DATETIME          (created_time)	\
	FIELD_DATETIME          (expiry_time)	\
	FIELD_INTEGER_UNSIGNED  (type)	\
	FIELD_STRING            (metadata)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   Center_MapEventBlock
#define MYSQL_OBJECT_FIELDS \
	FIELD_BIGINT            (x)	\
	FIELD_BIGINT            (y)	\
	FIELD_DATETIME          (updated_time)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
