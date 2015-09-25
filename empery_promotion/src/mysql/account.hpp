#ifndef EMPERY_PROMOTION_MYSQL_ACCOUNT_HPP_
#define EMPERY_PROMOTION_MYSQL_ACCOUNT_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyPromotion {

namespace MySql {

#define MYSQL_OBJECT_NAME	Promotion_Account
#define MYSQL_OBJECT_FIELDS	\
	FIELD_BIGINT_UNSIGNED	(accountId)	\
	FIELD_STRING			(loginName)	\
	FIELD_STRING			(nick)	\
	FIELD_STRING			(passwordHash)	\
	FIELD_STRING			(dealPasswordHash)	\
	FIELD_BIGINT_UNSIGNED	(referrerId)	\
	FIELD_BIGINT_UNSIGNED	(flags)	\
	FIELD_DATETIME			(bannedUntil)	\
	FIELD_DATETIME			(createdTime)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
