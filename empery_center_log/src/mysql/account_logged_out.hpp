#ifndef EMPERY_CENTER_LOG_MYSQL_ACCOUNT_LOGGED_OUT_HPP_
#define EMPERY_CENTER_LOG_MYSQL_ACCOUNT_LOGGED_OUT_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenterLog {

namespace MySql {

#define MYSQL_OBJECT_NAME   CenterLog_AccountLoggedOut
#define MYSQL_OBJECT_FIELDS \
	FIELD_DATETIME          (timestamp)	\
	FIELD_UUID              (account_uuid)	\
	FIELD_BIGINT_UNSIGNED   (online_duration)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
