#ifndef EMPERY_CENTER_MYSQL_TASK_HPP_
#define EMPERY_CENTER_MYSQL_TASK_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenter {

namespace MySql {

#define MYSQL_OBJECT_NAME   Center_Task
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (account_uuid)	\
	FIELD_INTEGER_UNSIGNED  (task_id)	\
	FIELD_INTEGER_UNSIGNED  (category)	\
	FIELD_DATETIME          (created_time)	\
	FIELD_DATETIME          (expiry_time)	\
	FIELD_STRING            (progress)	\
	FIELD_BOOLEAN           (rewarded)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   Center_LegionTask
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (legion_uuid) \
	FIELD_INTEGER_UNSIGNED  (task_id)	\
	FIELD_INTEGER_UNSIGNED  (category)	\
	FIELD_DATETIME          (created_time)	\
	FIELD_DATETIME          (expiry_time)	\
	FIELD_STRING            (progress)	\
	FIELD_BOOLEAN           (rewarded)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   Center_LegionTaskReward
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (account_uuid) \
	FIELD_INTEGER_UNSIGNED  (task_type_id)	\
	FIELD_STRING            (progress)	\
	FIELD_DATETIME          (created_time) \
	FIELD_DATETIME          (last_reward_time)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   Center_LegionTaskContribution
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (legion_uuid) \
	FIELD_UUID              (account_uuid) \
	FIELD_INTEGER_UNSIGNED  (day_contribution)	\
	FIELD_INTEGER_UNSIGNED  (week_contribution)	\
	FIELD_INTEGER_UNSIGNED  (total_contribution)	\
	FIELD_DATETIME          (last_update_time)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
