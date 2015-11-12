#ifndef EMPERY_PROMOTION_LOG_MYSQL_ACCOUNT_LOGGED_IN_HPP_
#define EMPERY_PROMOTION_LOG_MYSQL_ACCOUNT_LOGGED_IN_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyPromotionLog {

namespace MySql {

#define MYSQL_OBJECT_NAME   PromotionLog_AccountLoggedIn
#define MYSQL_OBJECT_FIELDS \
	FIELD_DATETIME          (timestamp)	\
	FIELD_BIGINT_UNSIGNED   (accountId)	\
	FIELD_STRING            (remoteIp)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
