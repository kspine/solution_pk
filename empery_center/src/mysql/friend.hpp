#ifndef EMPERY_CENTER_MYSQL_FRIEND_HPP_
#define EMPERY_CENTER_MYSQL_FRIEND_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenter {

namespace MySql {

#define MYSQL_OBJECT_NAME   Center_Friend
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (account_uuid)	\
	FIELD_UUID              (friend_uuid)	\
	FIELD_INTEGER_UNSIGNED  (category)	\
	FIELD_STRING            (metadata)	\
	FIELD_DATETIME          (updated_time)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   Center_FriendRecord
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (auto_uuid)	\
	FIELD_UUID              (account_uuid)	\
	FIELD_DATETIME          (timestamp)	\
	FIELD_UUID              (friend_account_uuid)	\
	FIELD_INTEGER           (result_type)	\
	FIELD_BOOLEAN           (deleted)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
