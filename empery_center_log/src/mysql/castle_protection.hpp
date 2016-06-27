#ifndef EMPERY_CENTER_LOG_MYSQL_CASTLE_PROTECTION_HPP_
#define EMPERY_CENTER_LOG_MYSQL_CASTLE_PROTECTION_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenterLog {

namespace MySql {

#define MYSQL_OBJECT_NAME   CenterLog_CastleProtection
#define MYSQL_OBJECT_FIELDS \
	FIELD_DATETIME          (timestamp)	\
	FIELD_UUID              (map_object_uuid)	\
	FIELD_UUID              (owner_uuid)	\
	FIELD_BIGINT_UNSIGNED   (delta_preparation_duration)	\
	FIELD_BIGINT_UNSIGNED   (delta_protection_duration)	\
	FIELD_DATETIME          (protection_end)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
