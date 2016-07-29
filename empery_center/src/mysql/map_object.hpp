#ifndef EMPERY_CENTER_MYSQL_MAP_OBJECT_HPP_
#define EMPERY_CENTER_MYSQL_MAP_OBJECT_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenter {

namespace MySql {

#define MYSQL_OBJECT_NAME   Center_MapObject
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (map_object_uuid)	\
	FIELD_INTEGER_UNSIGNED  (map_object_type_id)	\
	FIELD_UUID              (owner_uuid)	\
	FIELD_UUID              (parent_object_uuid)	\
	FIELD_STRING            (name)	\
	FIELD_BIGINT			(x)	\
	FIELD_BIGINT			(y)	\
	FIELD_DATETIME          (created_time)	\
	FIELD_DATETIME          (expiry_time)	\
	FIELD_BOOLEAN           (garrisoned)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   Center_MapObjectAttribute
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (map_object_uuid)	\
	FIELD_INTEGER_UNSIGNED  (attribute_id)	\
	FIELD_BIGINT            (value)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   Center_MapObjectBuff
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (map_object_uuid)	\
	FIELD_INTEGER_UNSIGNED  (buff_id)	\
	FIELD_BIGINT_UNSIGNED   (duration)	\
	FIELD_DATETIME          (time_begin)	\
	FIELD_DATETIME          (time_end)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
