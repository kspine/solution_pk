#ifndef EMPERY_CENTER_MYSQL_PAYMENT_TRANSACTION_HPP_
#define EMPERY_CENTER_MYSQL_PAYMENT_TRANSACTION_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenter {

namespace MySql {

#define MYSQL_OBJECT_NAME   Center_PaymentTransaction
#define MYSQL_OBJECT_FIELDS \
	FIELD_STRING            (serial)	\
	FIELD_UUID              (account_uuid)	\
	FIELD_DATETIME          (created_time)	\
	FIELD_DATETIME          (expiry_time)	\
	FIELD_BIGINT_UNSIGNED   (amount)	\
	FIELD_STRING            (remarks)	\
	FIELD_TINYINT_UNSIGNED  (settled)	\
	FIELD_TINYINT_UNSIGNED  (cancelled)	\
	FIELD_STRING            (operation_remarks)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
