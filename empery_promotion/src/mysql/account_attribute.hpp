#ifndef EMPERY_PROMOTION_MYSQL_ACCOUNT_ATTRIBUTE_HPP_
#define EMPERY_PROMOTION_MYSQL_ACCOUNT_ATTRIBUTE_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyPromotion {

namespace MySql {

#define MYSQL_OBJECT_NAME	Promotion_AccountAttribute
#define MYSQL_OBJECT_FIELDS	\
	FIELD_BIGINT_UNSIGNED	(accountId)	\
	FIELD_INTEGER_UNSIGNED	(slot)	\
	FIELD_STRING			(value)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
