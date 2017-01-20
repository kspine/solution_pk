#ifndef EMPERY_CENTER_MYSQL_LEGION_HPP_
#define EMPERY_CENTER_MYSQL_LEGION_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenter {
	namespace MySql {
#define MYSQL_OBJECT_NAME   Center_Legion
#define MYSQL_OBJECT_FIELDS \
    FIELD_UUID              (legion_uuid)	\
    FIELD_STRING            (name)	\
    FIELD_UUID            	(creater_uuid)	\
    FIELD_DATETIME          (created_time)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   Center_LegionAttribute
#define MYSQL_OBJECT_FIELDS \
    FIELD_UUID              (legion_uuid)	\
    FIELD_INTEGER_UNSIGNED  (legion_attribute_id)	\
    FIELD_STRING            (value)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   Center_Legion_Member
#define MYSQL_OBJECT_FIELDS \
    FIELD_UUID            	(account_uuid)	\
    FIELD_UUID              (legion_uuid)	\
    FIELD_DATETIME          (join_time)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   Center_LegionMemberAttribute
#define MYSQL_OBJECT_FIELDS \
    FIELD_UUID              (account_uuid)	\
    FIELD_INTEGER_UNSIGNED  (legion_member_attribute_id)	\
    FIELD_STRING            (value)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   Center_LegionApplyJoin
#define MYSQL_OBJECT_FIELDS \
    FIELD_UUID              (account_uuid)	\
    FIELD_UUID  			(legion_uuid)	\
    FIELD_DATETIME          (apply_time)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   Center_LegionInviteJoin
#define MYSQL_OBJECT_FIELDS \
    FIELD_UUID              (legion_uuid)	\
    FIELD_UUID              (account_uuid)	\
    FIELD_UUID  			(invited_uuid)	\
    FIELD_DATETIME          (invite_time)
#include <poseidon/mysql/object_generator.hpp>

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
    FIELD_INTEGER_UNSIGNED  (share_package_status) \
    FIELD_DATETIME          (share_package_pick_time)
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

#define MYSQL_OBJECT_NAME   Center_LegionFinancial
#define MYSQL_OBJECT_FIELDS \
    FIELD_UUID               (financial_uuid)  \
    FIELD_UUID               (legion_uuid)  \
    FIELD_UUID               (account_uuid)  \
    FIELD_INTEGER_UNSIGNED   (item_id) \
    FIELD_BIGINT_UNSIGNED    (old_count) \
    FIELD_BIGINT_UNSIGNED    (new_count) \
    FIELD_BIGINT             (delta_count) \
    FIELD_INTEGER_UNSIGNED   (action_id)   \
    FIELD_INTEGER_UNSIGNED   (action_count) \
    FIELD_DATETIME           (created_time)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   Center_LegionDonate
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (legion_uuid) \
	FIELD_UUID              (account_uuid) \
	FIELD_INTEGER_UNSIGNED  (day_donate)	\
	FIELD_INTEGER_UNSIGNED  (week_donate)	\
	FIELD_INTEGER_UNSIGNED  (total_donate)	\
	FIELD_DATETIME          (last_update_time)
#include <poseidon/mysql/object_generator.hpp>

}
}

#endif