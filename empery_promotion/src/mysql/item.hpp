#ifndef EMPERY_PROMOTION_MYSQL_ITEM_HPP_
#define EMPERY_PROMOTION_MYSQL_ITEM_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyPromotion {

namespace MySql {

#define MYSQL_OBJECT_NAME   Promotion_Item
#define MYSQL_OBJECT_FIELDS \
	FIELD_BIGINT_UNSIGNED   (account_id)	\
	FIELD_INTEGER_UNSIGNED  (item_id)	\
	FIELD_BIGINT_UNSIGNED   (count)	\
	FIELD_DATETIME          (updated_time)	\
	FIELD_BIGINT_UNSIGNED   (flags)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
