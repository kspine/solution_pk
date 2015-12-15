#ifndef EMPERY_CENTER_MYSQL_OVERLAY_HPP_
#define EMPERY_CENTER_MYSQL_OVERLAY_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenter {

namespace MySql {

#define MYSQL_OBJECT_NAME   Center_Overlay
#define MYSQL_OBJECT_FIELDS \
	FIELD_BIGINT            (cluster_x)	\
	FIELD_BIGINT            (cluster_y)	\
	FIELD_STRING            (overlay_group)	\
	FIELD_INTEGER_UNSIGNED  (overlay_id)	\
	FIELD_BIGINT_UNSIGNED   (resource_amount)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
