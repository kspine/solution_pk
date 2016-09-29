#ifndef EMPERY_CENTER_LOG_MYSQL_LEGION_HPP_
#define EMPERY_CENTER_LOG_MYSQL_LEGION_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenterLog {

namespace MySql {

#define MYSQL_OBJECT_NAME   CenterLog_LegionDisband
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (account_uuid)	\
	FIELD_STRING  			(legion_name) \
	FIELD_DATETIME          (disband_time)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   CenterLog_LeagueDisband
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (account_uuid)	\
	FIELD_STRING  			(league_name) \
	FIELD_DATETIME          (disband_time)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   CenterLog_LegionMemberTrace
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (account_uuid)	\
	FIELD_UUID              (legion_uuid)	\
	FIELD_UUID              (action_uuid)	\
	FIELD_INTEGER_UNSIGNED  (action_type)	\
	FIELD_DATETIME          (created_time)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   CenterLog_LeagueLegionTrace
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (legion_uuid)	\
	FIELD_UUID              (league_uuid)	\
	FIELD_UUID              (action_uuid)	\
	FIELD_INTEGER_UNSIGNED  (action_type)	\
	FIELD_DATETIME          (created_time)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
