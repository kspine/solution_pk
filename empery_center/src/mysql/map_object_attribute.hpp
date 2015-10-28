#ifndef EMPERY_CENTER_MYSQL_MAP_OBJECT_ATTRIBUTE_HPP_
#define EMPERY_CENTER_MYSQL_MAP_OBJECT_ATTRIBUTE_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenter {

namespace MySql {

#define MYSQL_OBJECT_NAME	Center_MapObjectAttribute
#define MYSQL_OBJECT_FIELDS	\
	FIELD_UUID				(mapObjectUuid)	\
	FIELD_INTEGER_UNSIGNED	(mapObjectAttrId)	\
	FIELD_BIGINT			(value)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
