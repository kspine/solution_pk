#ifndef EMPERY_CENTER_LOG_MYSQL_MAP_EVENTS_OVERFLOWED_HPP_
#define EMPERY_CENTER_LOG_MYSQL_MAP_EVENTS_OVERFLOWED_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenterLog {

namespace MySql {

#define MYSQL_OBJECT_NAME   CenterLog_MapEventsOverflowed
#define MYSQL_OBJECT_FIELDS \
	FIELD_DATETIME          (timestamp)	\
	FIELD_BIGINT            (block_x)	\
	FIELD_BIGINT            (block_y)	\
	FIELD_INTEGER_UNSIGNED  (width)	\
	FIELD_INTEGER_UNSIGNED  (height)	\
	FIELD_BIGINT_UNSIGNED   (events_to_refresh)	\
	FIELD_BIGINT_UNSIGNED   (events_retained)	\
	FIELD_BIGINT_UNSIGNED   (events_created)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
