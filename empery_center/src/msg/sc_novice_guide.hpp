#ifndef EMPERY_CENTER_MSG_SC_NOVICE_GUIDE_HPP_
#define EMPERY_CENTER_MSG_SC_NOVICE_GUIDE_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

  namespace Msg {

        #define MESSAGE_NAME SC_GetTaskStepInfoResMessage
        #define MESSAGE_ID 2002
        #define MESSAGE_FIELDS \
        FIELD_VUINT (task_id) \
        FIELD_VUINT (step_id)
        #include <poseidon/cbpp/message_generator.hpp>
   }
}
#endif