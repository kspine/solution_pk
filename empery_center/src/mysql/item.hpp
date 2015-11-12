#ifndef EMPERY_CENTER_MYSQL_ITEM_HPP_
#define EMPERY_CENTER_MYSQL_ITEM_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenter {

namespace MySql {

#define MYSQL_OBJECT_NAME   Center_Item
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (accountUuid)	\
	FIELD_INTEGER_UNSIGNED  (itemId)	\
	FIELD_BIGINT_UNSIGNED   (count)	\
	FIELD_DATETIME          (updatedTime)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
