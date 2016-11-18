#ifndef EMPERY_CENTER_LOG_MYSQL_NOVICE_GUIDE_HPP_
#define EMPERY_CENTER_LOG_MYSQL_NOVICE_GUIDE_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenterLog {

namespace MySql {

#define MYSQL_OBJECT_NAME   CenterLog_NoviceGuideTrace
#define MYSQL_OBJECT_FIELDS \
    FIELD_UUID              (account_uuid)  \
    FIELD_INTEGER_UNSIGNED  (task_id)   \
    FIELD_INTEGER_UNSIGNED  (step_id)   \
    FIELD_DATETIME          (created_time)
#include <poseidon/mysql/object_generator.hpp>
}
}
#endif
