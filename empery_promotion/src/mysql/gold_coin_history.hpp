#ifndef EMPERY_PROMOTION_MYSQL_GOLD_COIN_HISTORY_HPP_
#define EMPERY_PROMOTION_MYSQL_GOLD_COIN_HISTORY_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyPromotion {

namespace MySql {

#define MYSQL_OBJECT_NAME   Promotion_GoldCoinHistory
#define MYSQL_OBJECT_FIELDS \
	FIELD_BIGINT_UNSIGNED   (account_id)	\
	FIELD_DATETIME          (timestamp)	\
	FIELD_INTEGER_UNSIGNED  (auto_id)	\
	FIELD_BIGINT_UNSIGNED   (old_count)	\
	FIELD_BIGINT_UNSIGNED   (new_count)	\
	FIELD_INTEGER_UNSIGNED  (reason)	\
	FIELD_BIGINT_UNSIGNED   (param1)	\
	FIELD_BIGINT_UNSIGNED   (param2)	\
	FIELD_BIGINT_UNSIGNED   (param3)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
