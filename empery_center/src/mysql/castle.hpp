#ifndef EMPERY_CENTER_MYSQL_CASTLE_HPP_
#define EMPERY_CENTER_MYSQL_CASTLE_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenter {

namespace MySql {

#define MYSQL_OBJECT_NAME   Center_CastleBuildingBase
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (map_object_uuid)	\
	FIELD_INTEGER_UNSIGNED  (building_base_id)	\
	FIELD_INTEGER_UNSIGNED  (building_id)	\
	FIELD_BIGINT_UNSIGNED   (building_level)	\
	FIELD_INTEGER_UNSIGNED  (mission)	\
	FIELD_DATETIME          (mission_time_begin)	\
	FIELD_BIGINT_UNSIGNED   (mission_duration)	\
	FIELD_DATETIME          (mission_time_end)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   Center_CastleTech
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (map_object_uuid)	\
	FIELD_INTEGER_UNSIGNED  (tech_id)	\
	FIELD_BIGINT_UNSIGNED   (tech_level)	\
	FIELD_INTEGER_UNSIGNED  (mission)	\
	FIELD_DATETIME          (mission_time_begin)	\
	FIELD_BIGINT_UNSIGNED   (mission_duration)	\
	FIELD_DATETIME          (mission_time_end)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   Center_CastleResource
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (map_object_uuid)	\
	FIELD_INTEGER_UNSIGNED  (resource_id)	\
	FIELD_BIGINT_UNSIGNED   (count)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
