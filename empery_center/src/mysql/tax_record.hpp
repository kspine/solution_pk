#ifndef EMPERY_CENTER_MYSQL_TAX_RECORD_HPP_
#define EMPERY_CENTER_MYSQL_TAX_RECORD_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenter {

namespace MySql {

#define MYSQL_OBJECT_NAME   Center_TaxRecord
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (auto_uuid)	\
	FIELD_UUID              (account_uuid)	\
	FIELD_DATETIME          (timestamp)	\
	FIELD_UUID              (from_account_uuid)	\
	FIELD_INTEGER_UNSIGNED  (reason)	\
	FIELD_BIGINT_UNSIGNED   (old_amount)	\
	FIELD_BIGINT_UNSIGNED   (new_amount)	\
	FIELD_BIGINT            (delta_amount)	\
	FIELD_BOOLEAN           (deleted)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
