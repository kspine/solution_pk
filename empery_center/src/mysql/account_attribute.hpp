#ifndef EMPERY_CENTER_MYSQL_ACCOUNT_ATTRIBUTE_HPP_
#define EMPERY_CENTER_MYSQL_ACCOUNT_ATTRIBUTE_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenter {

namespace MySql {

#define MYSQL_OBJECT_NAME	Center_AccountAttribute
#define MYSQL_OBJECT_FIELDS	\
	FIELD_UUID				(accountUuid)	\
	FIELD_INTEGER_UNSIGNED	(slot)	\
	FIELD_STRING			(value)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
