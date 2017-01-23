#ifndef EMPERY_PROMOTION_LOG_MYSQL_SYNUSER_HPP_
#define EMPERY_PROMOTION_LOG_MYSQL_SYNUSER_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyPromotionLog {

namespace MySql {

#define MYSQL_OBJECT_NAME   PromotionLog_SynUser
#define MYSQL_OBJECT_FIELDS \
	FIELD_DATETIME          (timestamp)	\
	FIELD_STRING            (remote_ip)	\
	FIELD_STRING            (url)	\
	FIELD_STRING            (params)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
