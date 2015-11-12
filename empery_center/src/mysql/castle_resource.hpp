#ifndef EMPERY_CENTER_MYSQL_CASTLE_RESOURCE_HPP_
#define EMPERY_CENTER_MYSQL_CASTLE_RESOURCE_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenter {

namespace MySql {

#define MYSQL_OBJECT_NAME   Center_CastleResource
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (mapObjectUuid)	\
	FIELD_INTEGER_UNSIGNED  (resourceId)	\
	FIELD_BIGINT_UNSIGNED   (count)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
