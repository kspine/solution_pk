#ifndef EMPERY_PROMOTION_MYSQL_WITHDRAWAL_SLIP_HPP_
#define EMPERY_PROMOTION_MYSQL_WITHDRAWAL_SLIP_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyPromotion {

namespace MySql {

#define MYSQL_OBJECT_NAME	Promotion_WdSlip
#define MYSQL_OBJECT_FIELDS	\
	FIELD_STRING			(serial)	\
	FIELD_DATETIME			(createdTime)	\
	FIELD_BIGINT_UNSIGNED	(amount)	\
	FIELD_BIGINT_UNSIGNED	(fee)	\
	FIELD_BIGINT_UNSIGNED	(accountId)	\
	FIELD_INTEGER_UNSIGNED	(state)	\
	FIELD_STRING			(callbackIp)	\
	FIELD_STRING			(remarks)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
