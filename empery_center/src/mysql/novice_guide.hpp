#ifndef EMPERY_CENTER_MYSQL_NOVICE_GUIDE_HPP_
#define EMPERY_CENTER_MYSQL_NOVICE_GUIDE_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenter{
   namespace Mysql{
      #define MYSQL_OBJECT_NAME Center_NoviceGuide
      #define MYSQL_OBJECT_FIELDS \
                  FIELD_UUID (account_uuid) \
                  FIELD_INTEGER_UNSIGNED (task_id) \
                  FIELD_INTEGER_UNSIGNED (step_id)
      #include <poseidon/mysql/object_generator.hpp>
   }
}




































#endif