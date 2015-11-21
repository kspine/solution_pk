#ifndef EMPERY_CENTER_MYSQL_MAP_CELL_HPP_
#define EMPERY_CENTER_MYSQL_MAP_CELL_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenter {

namespace MySql {

#define MYSQL_OBJECT_NAME   Center_MapCell
#define MYSQL_OBJECT_FIELDS \
	FIELD_BIGINT            (x)	\
	FIELD_BIGINT            (y)	\
	FIELD_UUID              (parent_object_uuid)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   Center_MapCellAttribute
#define MYSQL_OBJECT_FIELDS \
	FIELD_BIGINT            (x)	\
	FIELD_BIGINT            (y)	\
	FIELD_INTEGER_UNSIGNED  (attribute_id)	\
	FIELD_BIGINT            (value)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
