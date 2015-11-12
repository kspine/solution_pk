#ifndef EMPERY_PROMOTION_MYSQL_SUM_ROWS_HPP_
#define EMPERY_PROMOTION_MYSQL_SUM_ROWS_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyPromotion {

namespace MySql {

#define MYSQL_OBJECT_NAME   Promotion_SumRows
#define MYSQL_OBJECT_FIELDS \
	FIELD_BIGINT_UNSIGNED   (sum)	\
	FIELD_BIGINT_UNSIGNED   (rows)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
