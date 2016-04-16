#ifndef EMPERY_CENTER_MYSQL_BATTLE_RECORD_HPP_
#define EMPERY_CENTER_MYSQL_BATTLE_RECORD_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenter {

namespace MySql {

#define MYSQL_OBJECT_NAME   Center_BattleRecord
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (auto_uuid)	\
	FIELD_UUID              (first_account_uuid)	\
	FIELD_DATETIME          (timestamp)	\
	FIELD_INTEGER_UNSIGNED  (first_object_type_id)	\
	FIELD_BIGINT            (first_coord_x)	\
	FIELD_BIGINT            (first_coord_y)	\
	FIELD_UUID              (second_account_uuid)	\
	FIELD_INTEGER_UNSIGNED  (second_object_type_id)	\
	FIELD_BIGINT            (second_coord_x)	\
	FIELD_BIGINT            (second_coord_y)	\
	FIELD_INTEGER           (result_type)	\
	FIELD_BIGINT_UNSIGNED   (soldiers_wounded)	\
	FIELD_BIGINT            (result_param2)	\
	FIELD_BIGINT_UNSIGNED   (soldiers_damaged)	\
	FIELD_BIGINT_UNSIGNED   (soldiers_remaining)	\
	FIELD_BOOLEAN           (deleted)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
