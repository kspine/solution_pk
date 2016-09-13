#ifndef EMPERY_CENTER_MYSQL_ACTIVATION_CODE_HPP_
#define EMPERY_CENTER_MYSQL_ACTIVATION_CODE_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenter {

namespace MySql {

#define MYSQL_OBJECT_NAME   Center_ActivationCode
#define MYSQL_OBJECT_FIELDS \
	FIELD_STRING            (code)	\
	FIELD_DATETIME          (created_time)	\
	FIELD_DATETIME          (expiry_time)	\
	FIELD_UUID              (used_by_account)	\
	FIELD_DATETIME          (used_time)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
