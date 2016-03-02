#ifndef EMPERY_CENTER_MYSQL_MAP_EVENT_HPP_
#define EMPERY_CENTER_MYSQL_MAP_EVENT_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenter {

namespace MySql {

#define MYSQL_OBJECT_NAME   Center_MapEventBlock
#define MYSQL_OBJECT_FIELDS \
	FIELD_BIGINT            (block_x)	\
	FIELD_BIGINT            (block_y)	\
	FIELD_DATETIME          (next_refresh_time)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   Center_MapEvent
#define MYSQL_OBJECT_FIELDS \
	FIELD_BIGINT            (x)	\
	FIELD_BIGINT            (y)	\
	FIELD_DATETIME          (created_time)	\
	FIELD_DATETIME          (expiry_time)	\
	FIELD_INTEGER_UNSIGNED  (map_event_id)	\
	FIELD_UUID              (meta_uuid)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
