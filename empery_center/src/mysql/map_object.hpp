#ifndef EMPERY_CENTER_MYSQL_MAP_OBJECT_HPP_
#define EMPERY_CENTER_MYSQL_MAP_OBJECT_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenter {

namespace MySql {

#define MYSQL_OBJECT_NAME   Center_MapObject
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (mapObjectUuid)	\
	FIELD_INTEGER_UNSIGNED  (mapObjectTypeId)	\
	FIELD_UUID              (ownerUuid)	\
	FIELD_STRING            (name)	\
	FIELD_DATETIME          (createdTime)	\
	FIELD_TINYINT_UNSIGNED  (deleted)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
