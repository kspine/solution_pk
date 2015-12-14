#ifndef EMPERY_CENTER_LOG_MYSQL_RESOURCE_CHANGED_HPP_
#define EMPERY_CENTER_LOG_MYSQL_RESOURCE_CHANGED_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenterLog {

namespace MySql {

#define MYSQL_OBJECT_NAME   CenterLog_ResourceChanged
#define MYSQL_OBJECT_FIELDS \
	FIELD_DATETIME          (timestamp)	\
	FIELD_UUID              (map_object_uuid)	\
	FIELD_UUID              (owner_uuid)	\
	FIELD_INTEGER_UNSIGNED  (resource_id)	\
	FIELD_BIGINT_UNSIGNED   (old_amount)	\
	FIELD_BIGINT_UNSIGNED   (new_amount)	\
	FIELD_BIGINT            (delta_amount)	\
	FIELD_INTEGER_UNSIGNED  (reason)	\
	FIELD_BIGINT            (param1)	\
	FIELD_BIGINT            (param2)	\
	FIELD_BIGINT            (param3)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
