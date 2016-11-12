#ifndef EMPERY_CENTER_LOG_MYSQL_DUNGEON_FINISH_HPP_
#define EMPERY_CENTER_LOG_MYSQL_DUNGEON_FINISH_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenterLog {

namespace MySql {

#define MYSQL_OBJECT_NAME   CenterLog_DungeonFinish
#define MYSQL_OBJECT_FIELDS \
	FIELD_DATETIME          (timestamp)	\
	FIELD_UUID              (account_uuid)	\
	FIELD_INTEGER_UNSIGNED  (dungeon_type_id)	\
	FIELD_INTEGER_UNSIGNED  (begin_time)	\
	FIELD_INTEGER_UNSIGNED  (finish_time)	\
	FIELD_INTEGER_UNSIGNED  (finished)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
