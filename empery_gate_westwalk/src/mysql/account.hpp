#ifndef EMPERY_GATE_WESTWALK_MYSQL_ACCOUNT_HPP_
#define EMPERY_GATE_WESTWALK_MYSQL_ACCOUNT_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyGateWestwalk {

namespace MySql {

#define MYSQL_OBJECT_NAME   Westwalk_Account
#define MYSQL_OBJECT_FIELDS \
	FIELD_STRING            (accountName)	\
	FIELD_STRING            (createdIp)	\
	FIELD_DATETIME          (createdTime)	\
	FIELD_STRING            (remarks)	\
	FIELD_STRING            (passwordHash)	\
	FIELD_STRING            (disposablePasswordHash)	\
	FIELD_DATETIME          (disposablePasswordExpiryTime)	\
	FIELD_DATETIME          (passwordRegainCooldownTime)	\
	FIELD_STRING            (token)	\
	FIELD_DATETIME          (tokenExpiryTime)	\
	FIELD_STRING            (lastLoginIp)	\
	FIELD_DATETIME          (lastLoginTime)	\
	FIELD_DATETIME          (bannedUntil)	\
	FIELD_INTEGER_UNSIGNED  (retryCount)	\
	FIELD_BIGINT_UNSIGNED   (flags)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
