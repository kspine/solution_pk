#ifndef EMPERY_GATE_WESTWALK_MYSQL_ACCOUNT_HPP_
#define EMPERY_GATE_WESTWALK_MYSQL_ACCOUNT_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyGateWestwalk {

namespace MySql {

#define MYSQL_OBJECT_NAME   Westwalk_Account
#define MYSQL_OBJECT_FIELDS \
	FIELD_STRING            (account_name)	\
	FIELD_STRING            (created_ip)	\
	FIELD_DATETIME          (created_time)	\
	FIELD_STRING            (remarks)	\
	FIELD_STRING            (password_hash)	\
	FIELD_STRING            (disposable_password_hash)	\
	FIELD_DATETIME          (disposable_password_expiry_time)	\
	FIELD_DATETIME          (password_regain_cooldown_time)	\
	FIELD_STRING            (token)	\
	FIELD_DATETIME          (token_expiry_time)	\
	FIELD_STRING            (last_login_ip)	\
	FIELD_DATETIME          (last_login_time)	\
	FIELD_DATETIME          (banned_until)	\
	FIELD_INTEGER_UNSIGNED  (retry_count)	\
	FIELD_BIGINT_UNSIGNED   (flags)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
