#ifndef EMPERY_CENTER_MYSQL_ITEM_HPP_
#define EMPERY_CENTER_MYSQL_ITEM_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenter {

namespace MySql {

#define MYSQL_OBJECT_NAME   Center_Item
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (account_uuid)	\
	FIELD_INTEGER_UNSIGNED  (item_id)	\
	FIELD_BIGINT_UNSIGNED   (count)	\
	FIELD_DATETIME          (updated_time)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
