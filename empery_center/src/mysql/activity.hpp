#ifndef EMPERY_CENTER_MYSQL_ACTIVITY_HPP_
#define EMPERY_CENTER_MYSQL_ACTIVITY_HPP_

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

#define MYSQL_OBJECT_NAME   Center_MapActivityRank
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (account_uuid)	 \
	FIELD_BIGINT_UNSIGNED   (map_activity_id)\
	FIELD_DATETIME          (settle_date) \
	FIELD_INTEGER_UNSIGNED  (rank) \
	FIELD_BIGINT_UNSIGNED   (accumulate_value)\
	FIELD_DATETIME          (process_date)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   Center_MapWorldActivity
#define MYSQL_OBJECT_FIELDS \
	FIELD_BIGINT_UNSIGNED   (activity_id)\
	FIELD_BIGINT            (cluster_x)	\
	FIELD_BIGINT            (cluster_y)	\
	FIELD_DATETIME          (since)\
	FIELD_DATETIME          (sub_since)\
	FIELD_DATETIME          (sub_until)\
	FIELD_BIGINT_UNSIGNED   (accumulate_value)\
	FIELD_BOOLEAN           (finish)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   Center_MapWorldActivityAccumulate
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (account_uuid)	 \
	FIELD_BIGINT_UNSIGNED   (activity_id)\
	FIELD_BIGINT            (cluster_x)	\
	FIELD_BIGINT            (cluster_y)	\
	FIELD_DATETIME          (since) \
	FIELD_BIGINT_UNSIGNED   (accumulate_value) \
	FIELD_BOOLEAN           (rewarded)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   Center_MapWorldActivityRank
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (account_uuid)	 \
	FIELD_BIGINT            (cluster_x)	\
	FIELD_BIGINT            (cluster_y)	\
	FIELD_DATETIME          (since) \
	FIELD_INTEGER_UNSIGNED  (rank) \
	FIELD_BIGINT_UNSIGNED   (accumulate_value)\
	FIELD_DATETIME          (process_date) \
	FIELD_BOOLEAN           (rewarded)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   Center_MapCountryStatics
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (account_uuid) \
	FIELD_BIGINT_UNSIGNED   (accumulate_value)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   Center_MapWorldActivityBoss
#define MYSQL_OBJECT_FIELDS \
	FIELD_BIGINT            (cluster_x) \
	FIELD_BIGINT            (cluster_y) \
	FIELD_DATETIME          (since) \
	FIELD_UUID              (boss_uuid) \
	FIELD_DATETIME          (create_date) \
	FIELD_DATETIME          (delete_date) \
	FIELD_BIGINT_UNSIGNED   (hp_total) \
	FIELD_BIGINT_UNSIGNED   (hp_die)
#include <poseidon/mysql/object_generator.hpp>





}

}

#endif
