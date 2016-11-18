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
	FIELD_BIGINT_UNSIGNED   (mission_duration)	\
	FIELD_DATETIME          (mission_time_begin)	\
	FIELD_DATETIME          (mission_time_end)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   Center_CastleTech
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (map_object_uuid)	\
	FIELD_INTEGER_UNSIGNED  (tech_id)	\
	FIELD_BIGINT_UNSIGNED   (tech_level)	\
	FIELD_INTEGER_UNSIGNED  (mission)	\
	FIELD_BIGINT_UNSIGNED   (mission_duration)	\
	FIELD_DATETIME          (mission_time_begin)	\
	FIELD_DATETIME          (mission_time_end)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   Center_CastleResource
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (map_object_uuid)	\
	FIELD_INTEGER_UNSIGNED  (resource_id)	\
	FIELD_BIGINT_UNSIGNED   (amount)	\
	FIELD_DATETIME          (updated_time)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   Center_CastleBattalion
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (map_object_uuid)	\
	FIELD_INTEGER_UNSIGNED  (map_object_type_id)	\
	FIELD_BIGINT_UNSIGNED   (count)	\
	FIELD_BOOLEAN           (enabled)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   Center_CastleBattalionProduction
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (map_object_uuid)	\
	FIELD_INTEGER_UNSIGNED  (building_base_id)	\
	FIELD_INTEGER_UNSIGNED  (map_object_type_id)	\
	FIELD_BIGINT_UNSIGNED   (count)	\
	FIELD_BIGINT_UNSIGNED   (production_duration)	\
	FIELD_DATETIME          (production_time_begin)	\
	FIELD_DATETIME          (production_time_end)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   Center_CastleWoundedSoldier
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (map_object_uuid)	\
	FIELD_INTEGER_UNSIGNED  (map_object_type_id)	\
	FIELD_BIGINT_UNSIGNED   (count)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   Center_CastleTreatment
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (map_object_uuid)	\
	FIELD_INTEGER_UNSIGNED  (map_object_type_id)	\
	FIELD_BIGINT_UNSIGNED   (count)	\
	FIELD_BIGINT_UNSIGNED   (duration)	\
	FIELD_DATETIME          (time_begin)	\
	FIELD_DATETIME          (time_end)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   Center_CastleTechEra
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (map_object_uuid)	\
	FIELD_INTEGER_UNSIGNED  (tech_era)	\
	FIELD_BOOLEAN           (unlocked)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   Center_CastleResourceBattalionUnload
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (map_object_uuid)	\
	FIELD_INTEGER_UNSIGNED  (resource_id)	\
	FIELD_BIGINT_UNSIGNED   (delta)	\
	FIELD_DATETIME          (updated_time)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   Center_CastleOfflineUpgradeBuildingBase
#define MYSQL_OBJECT_FIELDS \
    FIELD_STRING            (auto_uuid) \
	FIELD_UUID              (account_uuid)	\
	FIELD_UUID              (map_object_uuid)	\
	FIELD_STRING            (building_base_ids)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
