#ifndef EMPERY_CENTER_LOG_MYSQL_RESOURCE_CHANGED_HPP_
#define EMPERY_CENTER_LOG_MYSQL_RESOURCE_CHANGED_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenterLog {

namespace MySql {

#define MYSQL_OBJECT_NAME	CenterLog_ResourceChanged
#define MYSQL_OBJECT_FIELDS	\
	FIELD_DATETIME			(timestamp)	\
	FIELD_UUID				(mapObjectUuid)	\
	FIELD_UUID				(ownerUuid)	\
	FIELD_INTEGER_UNSIGNED	(resourceId)	\
	FIELD_BIGINT_UNSIGNED	(oldCount)	\
	FIELD_BIGINT_UNSIGNED	(newCount)	\
	FIELD_INTEGER_UNSIGNED	(reason)	\
	FIELD_BIGINT_UNSIGNED	(param1)	\
	FIELD_BIGINT_UNSIGNED	(param2)	\
	FIELD_BIGINT_UNSIGNED	(param3)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
