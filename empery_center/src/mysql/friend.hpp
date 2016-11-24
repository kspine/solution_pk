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
	FIELD_DATETIME          (updated_time) \
	FIELD_INTEGER_UNSIGNED  (relation)
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

#define MYSQL_OBJECT_NAME   Center_FriendPrivateMsgData
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (msg_uuid)	\
	FIELD_INTEGER_UNSIGNED  (language_id)	\
	FIELD_DATETIME          (created_time)	\
	FIELD_UUID              (from_account_uuid)	\
	FIELD_STRING            (segments)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   Center_FriendPrivateMsgRecent
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (auto_uuid)	\
	FIELD_UUID              (account_uuid)	\
	FIELD_DATETIME          (timestamp)	\
	FIELD_UUID              (friend_account_uuid)	\
	FIELD_UUID              (msg_uuid)	\
	FIELD_BOOLEAN           (sender)	\
	FIELD_BOOLEAN           (read)	\
	FIELD_BOOLEAN           (deleted)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
