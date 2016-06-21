#ifndef EMPERY_CENTER_MYSQL_MAP_ACTIVITY_RANK_HPP_
#define EMPERY_CENTER_MYSQL_MAP_ACTIVITY_RANK_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenter {

namespace MySql {

#define MYSQL_OBJECT_NAME   Center_MapActivityRank
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (account_uuid)	 \
	FIELD_BIGINT_UNSIGNED   (map_activity_id)\
	FIELD_DATETIME          (settle_date) \
	FIELD_INTEGER_UNSIGNED  (rank) \
	FIELD_BIGINT_UNSIGNED   (accumulate_value)\
	FIELD_DATETIME          (process_date)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
