#ifndef EMPERY_CENTER_MYSQL_ACTIVITY_CONFIG_HPP_
#define EMPERY_CENTER_MYSQL_ACTIVITY_CONFIG_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenter {

namespace MySql {

#define MYSQL_OBJECT_NAME   Center_Activitys
#define MYSQL_OBJECT_FIELDS \
	FIELD_BIGINT_UNSIGNED   (activity_id)\
	FIELD_STRING            (name) \
	FIELD_DATETIME          (begin_time) \
	FIELD_DATETIME          (end_time)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   Center_ActivitysMap
#define MYSQL_OBJECT_FIELDS \
	FIELD_BIGINT_UNSIGNED   (activity_id)\
	FIELD_STRING            (name) \
	FIELD_INTEGER_UNSIGNED  (type) \
	FIELD_INTEGER_UNSIGNED  (duration) \
	FIELD_STRING            (target)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   Center_ActivitysRankAward
#define MYSQL_OBJECT_FIELDS \
	FIELD_BIGINT_UNSIGNED   (id)\
	FIELD_BIGINT_UNSIGNED   (activity_id)\
	FIELD_INTEGER_UNSIGNED  (rank_begin) \
	FIELD_INTEGER_UNSIGNED  (rank_end) \
	FIELD_STRING            (reward)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   Center_ActivitysWorld
#define MYSQL_OBJECT_FIELDS \
	FIELD_BIGINT_UNSIGNED   (activity_id)\
	FIELD_STRING            (name) \
	FIELD_BIGINT_UNSIGNED   (pro_activity_id)\
	FIELD_BIGINT_UNSIGNED   (own_activity_id)\
	FIELD_INTEGER_UNSIGNED  (type) \
	FIELD_STRING            (target) \
	FIELD_STRING            (drop)
#include <poseidon/mysql/object_generator.hpp>
}

}

#endif
