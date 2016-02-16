#ifndef EMPERY_CENTER_MYSQL_TASK_HPP_
#define EMPERY_CENTER_MYSQL_TASK_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenter {

namespace MySql {

#define MYSQL_OBJECT_NAME   Center_Task
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (account_uuid)	\
	FIELD_INTEGER_UNSIGNED  (task_id)	\
	FIELD_DATETIME          (created_time)	\
	FIELD_DATETIME          (expiry_time)	\
	FIELD_STRING            (progress)	\
	FIELD_BOOLEAN           (rewarded)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
