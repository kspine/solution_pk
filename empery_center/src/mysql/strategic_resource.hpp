#ifndef EMPERY_CENTER_MYSQL_STRATEGIC_RESOURCE_HPP_
#define EMPERY_CENTER_MYSQL_STRATEGIC_RESOURCE_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenter {

namespace MySql {

#define MYSQL_OBJECT_NAME   Center_StrategicResource
#define MYSQL_OBJECT_FIELDS \
	FIELD_BIGINT            (x)	\
	FIELD_BIGINT            (y)	\
	FIELD_INTEGER_UNSIGNED  (resource_id)	\
	FIELD_BIGINT_UNSIGNED   (resource_amount)	\
	FIELD_DATETIME          (created_time)	\
	FIELD_DATETIME          (expiry_time)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
