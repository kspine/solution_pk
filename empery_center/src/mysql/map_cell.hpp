#ifndef EMPERY_CENTER_MYSQL_MAP_CELL_HPP_
#define EMPERY_CENTER_MYSQL_MAP_CELL_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenter {

namespace MySql {

#define MYSQL_OBJECT_NAME   Center_MapCell
#define MYSQL_OBJECT_FIELDS \
	FIELD_BIGINT            (x)	\
	FIELD_BIGINT            (y)	\
	FIELD_UUID              (parent_object_uuid)	\
	FIELD_BOOLEAN           (acceleration_card_applied)	\
	FIELD_INTEGER_UNSIGNED  (ticket_item_id)	\
	FIELD_INTEGER_UNSIGNED  (production_resource_id)	\
	FIELD_DATETIME          (last_production_time)	\
	FIELD_BIGINT_UNSIGNED   (resource_amount)	\
	FIELD_UUID              (occupier_object_uuid)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   Center_MapCellAttribute
#define MYSQL_OBJECT_FIELDS \
	FIELD_BIGINT            (x)	\
	FIELD_BIGINT            (y)	\
	FIELD_INTEGER_UNSIGNED  (attribute_id)	\
	FIELD_BIGINT            (value)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   Center_MapCellBuff
#define MYSQL_OBJECT_FIELDS \
	FIELD_BIGINT            (x)	\
	FIELD_BIGINT            (y)	\
	FIELD_INTEGER_UNSIGNED  (buff_id)   \
	FIELD_BIGINT_UNSIGNED   (duration)  \
	FIELD_DATETIME          (time_begin)    \
	FIELD_DATETIME          (time_end)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
