#ifndef EMPERY_CENTER_MYSQL_ACCOUNT_HPP_
#define EMPERY_CENTER_MYSQL_ACCOUNT_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenter {

namespace MySql {

#define MYSQL_OBJECT_NAME   Center_Account
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (accountUuid)	\
	FIELD_INTEGER_UNSIGNED  (platformId)	\
	FIELD_STRING            (loginName)	\
	FIELD_STRING            (nick)	\
	FIELD_BIGINT_UNSIGNED   (flags)	\
	FIELD_STRING            (loginToken)	\
	FIELD_DATETIME          (loginTokenExpiryTime)	\
	FIELD_DATETIME          (bannedUntil)	\
	FIELD_DATETIME          (createdTime)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
