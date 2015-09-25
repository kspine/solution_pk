#ifndef EMPERY_PROMOTION_MYSQL_BILL_HPP_
#define EMPERY_PROMOTION_MYSQL_BILL_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyPromotion {

namespace MySql {

#define MYSQL_OBJECT_NAME	Promotion_Bill
#define MYSQL_OBJECT_FIELDS	\
	FIELD_STRING			(serial)	\
	FIELD_BIGINT_UNSIGNED	(createdTime)	\
	FIELD_BIGINT_UNSIGNED	(amount)	\
	FIELD_BIGINT_UNSIGNED	(accountId)	\
	FIELD_INTEGER_UNSIGNED	(state)	\
	FIELD_STRING			(callbackIp)	\
	FIELD_STRING			(remarks)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
