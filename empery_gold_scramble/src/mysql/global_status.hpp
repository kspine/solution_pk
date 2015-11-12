#ifndef EMPERY_GOLD_SCRAMBLE_MYSQL_GLOBAL_STATUS_HPP_
#define EMPERY_GOLD_SCRAMBLE_MYSQL_GLOBAL_STATUS_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyGoldScramble {

namespace MySql {

#define MYSQL_OBJECT_NAME   GoldScramble_GlobalStatus
#define MYSQL_OBJECT_FIELDS \
	FIELD_INTEGER_UNSIGNED  (slot)	\
	FIELD_BIGINT_UNSIGNED   (value)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
