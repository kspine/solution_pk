#ifndef EMPERY_CENTER_MYSQL_DUNGEON_HPP_
#define EMPERY_CENTER_MYSQL_DUNGEON_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenter {

namespace MySql {

#define MYSQL_OBJECT_NAME   Center_Dungeon
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (account_uuid)	\
	FIELD_INTEGER_UNSIGNED  (dungeon_type_id)	\
	FIELD_INTEGER_UNSIGNED  (score)	\
	FIELD_BIGINT_UNSIGNED   (entry_count)	\
	FIELD_BIGINT_UNSIGNED   (finish_count)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
