#ifndef EMPERY_PROMOTION_MYSQL_ACCELERATION_CARD_HISTORY_HPP_
#define EMPERY_PROMOTION_MYSQL_ACCELERATION_CARD_HISTORY_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyPromotion {

namespace MySql {

#define MYSQL_OBJECT_NAME	Promotion_AccelerationCardHistory
#define MYSQL_OBJECT_FIELDS	\
	FIELD_BIGINT_UNSIGNED	(accountId)	\
	FIELD_BIGINT_UNSIGNED	(timestamp)	\
	FIELD_BIGINT_UNSIGNED	(totalPrice)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
