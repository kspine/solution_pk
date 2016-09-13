#ifndef EMPERY_CENTER_MYSQL_DEFENSE_BUILDING_HPP_
#define EMPERY_CENTER_MYSQL_DEFENSE_BUILDING_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenter {

namespace MySql {

#define MYSQL_OBJECT_NAME   Center_DefenseBuilding
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (map_object_uuid)	\
	FIELD_BIGINT_UNSIGNED   (building_level)	\
	FIELD_INTEGER_UNSIGNED  (mission)	\
	FIELD_BIGINT_UNSIGNED   (mission_duration)	\
	FIELD_DATETIME          (mission_time_begin)	\
	FIELD_DATETIME          (mission_time_end)	\
	FIELD_UUID              (garrisoning_object_uuid)	\
	FIELD_DATETIME          (last_self_healed_time)
#include <poseidon/mysql/object_generator.hpp>


#define MYSQL_OBJECT_NAME   Center_WarehouseBuilding
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (map_object_uuid)	\
	FIELD_UUID              (legion_uuid)	\
	FIELD_BIGINT_UNSIGNED   (building_level)	\
	FIELD_INTEGER_UNSIGNED  (mission)	\
	FIELD_BIGINT_UNSIGNED   (mission_duration)	\
	FIELD_DATETIME          (mission_time_begin)	\
	FIELD_DATETIME          (mission_time_end)	\
	FIELD_UUID              (garrisoning_object_uuid)	\
	FIELD_DATETIME          (last_self_healed_time)	\
	FIELD_INTEGER_UNSIGNED  (output_type)	\
	FIELD_INTEGER_UNSIGNED  (output_amount)	\
	FIELD_DATETIME          (effect_time)	\
	FIELD_DATETIME          (cd_time)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
