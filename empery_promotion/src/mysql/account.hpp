#ifndef EMPERY_PROMOTION_MYSQL_ACCOUNT_HPP_
#define EMPERY_PROMOTION_MYSQL_ACCOUNT_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyPromotion {

namespace MySql {

#define MYSQL_OBJECT_NAME   Promotion_Account
#define MYSQL_OBJECT_FIELDS \
	FIELD_BIGINT_UNSIGNED   (account_id)	\
	FIELD_STRING            (login_name)	\
	FIELD_STRING            (phone_number)	\
	FIELD_STRING            (nick)	\
	FIELD_STRING            (password_hash)	\
	FIELD_STRING            (deal_password_hash)	\
	FIELD_BIGINT_UNSIGNED   (referrer_id)	\
	FIELD_BIGINT_UNSIGNED   (level)	\
	FIELD_BIGINT_UNSIGNED   (max_level)	\
	FIELD_BIGINT_UNSIGNED   (subordinate_count)	\
	FIELD_BIGINT_UNSIGNED   (flags)	\
	FIELD_DATETIME          (banned_until)	\
	FIELD_DATETIME          (created_time)	\
	FIELD_STRING            (created_ip)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
