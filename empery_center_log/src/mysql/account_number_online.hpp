#ifndef EMPERY_CENTER_LOG_MYSQL_ACCOUNT_NUMBER_ONLINE_HPP_
#define EMPERY_CENTER_LOG_MYSQL_ACCOUNT_NUMBER_ONLINE_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenterLog {

namespace MySql {

#define MYSQL_OBJECT_NAME   CenterLog_AccountNumberOnline
#define MYSQL_OBJECT_FIELDS \
	FIELD_DATETIME          (timestamp)	\
	FIELD_BIGINT_UNSIGNED   (interval)	\
	FIELD_BIGINT_UNSIGNED   (account_count)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
