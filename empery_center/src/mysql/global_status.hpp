#ifndef EMPERY_CENTER_MYSQL_GLOBAL_STATUS_HPP_
#define EMPERY_CENTER_MYSQL_GLOBAL_STATUS_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenter {

namespace MySql {

#define MYSQL_OBJECT_NAME   Center_GlobalStatus
#define MYSQL_OBJECT_FIELDS \
	FIELD_INTEGER_UNSIGNED  (slot)	\
	FIELD_STRING            (value)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
