#ifndef EMPERY_PROMOTION_MYSQL_SHARED_RECHARGE_HISTORY_HPP_
#define EMPERY_PROMOTION_MYSQL_SHARED_RECHARGE_HISTORY_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyPromotion {

namespace MySql {

#define MYSQL_OBJECT_NAME   Promotion_SharedRecharge
#define MYSQL_OBJECT_FIELDS \
	FIELD_BIGINT_UNSIGNED   (account_id)	\
	FIELD_BIGINT_UNSIGNED   (recharge_to_account_id)	\
	FIELD_DATETIME          (updated_time)	\
	FIELD_INTEGER_UNSIGNED  (state)	\
	FIELD_BIGINT_UNSIGNED   (amount)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
