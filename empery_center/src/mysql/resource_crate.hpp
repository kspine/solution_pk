#ifndef EMPERY_CENTER_MYSQL_RESOURCE_CRATE_HPP_
#define EMPERY_CENTER_MYSQL_RESOURCE_CRATE_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenter {

namespace MySql {

#define MYSQL_OBJECT_NAME   Center_ResourceCrate
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (resource_crate_uuid)	\
	FIELD_INTEGER_UNSIGNED  (resource_id)	\
	FIELD_BIGINT_UNSIGNED   (amount_max)	\
	FIELD_BIGINT			(x)	\
	FIELD_BIGINT			(y)	\
	FIELD_DATETIME          (created_time)	\
	FIELD_DATETIME          (expiry_time)	\
	FIELD_BIGINT_UNSIGNED   (amount_remaining)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
