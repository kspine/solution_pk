#ifndef EMPERY_CENTER_LOG_MYSQL_DUNGEON_DELETED_HPP_
#define EMPERY_CENTER_LOG_MYSQL_DUNGEON_DELETED_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenterLog {

namespace MySql {

#define MYSQL_OBJECT_NAME   CenterLog_DungeonDeleted
#define MYSQL_OBJECT_FIELDS \
	FIELD_DATETIME          (timestamp)	\
	FIELD_UUID              (account_uuid)	\
	FIELD_INTEGER_UNSIGNED  (dungeon_type_id)	\
	FIELD_INTEGER_UNSIGNED  (new_score)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
