#ifndef EMPERY_LEAGUE_MYSQL_LEAGUE_HPP_
#define EMPERY_LEAGUE_MYSQL_LEAGUE_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyLeague {

namespace MySql {

#define MYSQL_OBJECT_NAME   League_Info
#define MYSQL_OBJECT_FIELDS \
    FIELD_UUID              (league_uuid)	\
    FIELD_STRING            (name)	\
	FIELD_UUID            	(legion_uuid)	\
    FIELD_UUID            	(creater_uuid)	\
    FIELD_DATETIME          (created_time)
#include <poseidon/mysql/object_generator.hpp>


#define MYSQL_OBJECT_NAME   League_LeagueAttribute
#define MYSQL_OBJECT_FIELDS \
    FIELD_UUID              (league_uuid)	\
    FIELD_INTEGER_UNSIGNED  (league_attribute_id)	\
    FIELD_STRING            (value)
#include <poseidon/mysql/object_generator.hpp>


#define MYSQL_OBJECT_NAME   League_Member
#define MYSQL_OBJECT_FIELDS \
    FIELD_UUID            	(legion_uuid)	\
    FIELD_UUID              (league_uuid)	\
    FIELD_DATETIME          (join_time)
#include <poseidon/mysql/object_generator.hpp>


#define MYSQL_OBJECT_NAME   League_MemberAttribute
#define MYSQL_OBJECT_FIELDS \
    FIELD_UUID              (legion_uuid)	\
    FIELD_INTEGER_UNSIGNED  (league_member_attribute_id)	\
    FIELD_STRING            (value)
#include <poseidon/mysql/object_generator.hpp>


#define MYSQL_OBJECT_NAME   League_LeagueApplyJoin
#define MYSQL_OBJECT_FIELDS \
    FIELD_UUID              (legion_uuid)	\
    FIELD_UUID  			(league_uuid)	\
    FIELD_UUID  			(account_uuid)	\
    FIELD_DATETIME          (apply_time)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   League_LeagueInviteJoin
#define MYSQL_OBJECT_FIELDS \
    FIELD_UUID              (legion_uuid)	\
    FIELD_UUID              (league_uuid)	\
    FIELD_UUID  			(account_uuid)	\
    FIELD_DATETIME          (invite_time)
#include <poseidon/mysql/object_generator.hpp>

/*
#define MYSQL_OBJECT_NAME   Center_Legion_Package_Share
#define MYSQL_OBJECT_FIELDS \
    FIELD_UUID              (share_uuid)	\
    FIELD_UUID              (legion_uuid)	\
    FIELD_UUID              (account_uuid)	\
    FIELD_INTEGER_UNSIGNED  (task_id)	\
    FIELD_INTEGER_UNSIGNED  (task_package_item_id)	\
    FIELD_INTEGER_UNSIGNED  (share_package_item_id)	\
    FIELD_DATETIME          (share_package_time) \
    FIELD_DATETIME          (share_package_expire_time)

#include <poseidon/mysql/object_generator.hpp>


#define MYSQL_OBJECT_NAME   Center_Legion_Package_Pick_Share
#define MYSQL_OBJECT_FIELDS \
    FIELD_UUID              (share_uuid)	\
    FIELD_UUID              (account_uuid)	\
    FIELD_INTEGER_UNSIGNED  (share_package_status)
#include <poseidon/mysql/object_generator.hpp>


#define MYSQL_OBJECT_NAME   Center_LegionBuilding
#define MYSQL_OBJECT_FIELDS \
    FIELD_UUID              (legion_building_uuid)	\
    FIELD_UUID              (legion_uuid)	\
	FIELD_UUID              (map_object_uuid)	\
    FIELD_INTEGER_UNSIGNED  (ntype)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   Center_LegionBuildingAttribute
#define MYSQL_OBJECT_FIELDS \
    FIELD_UUID              (legion_building_uuid)	\
    FIELD_INTEGER_UNSIGNED  (legion_building_attribute_id)	\
    FIELD_STRING            (value)
#include <poseidon/mysql/object_generator.hpp>

*/
}

}

#endif
