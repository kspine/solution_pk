#ifndef EMPERY_CENTER_MYSQL_MAP_ACTIVITY_ACCMULATE_HPP_
#define EMPERY_CENTER_MYSQL_MAP_ACTIVITY_ACCMULATE_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenter {

namespace MySql {

#define MYSQL_OBJECT_NAME   Center_MapActivityAccumulate
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (account_uuid)	 \
	FIELD_BIGINT_UNSIGNED   (map_activity_id)\
	FIELD_DATETIME          (avaliable_since) \
	FIELD_DATETIME          (avaliable_util) \
	FIELD_BIGINT_UNSIGNED   (accumulate_value)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
