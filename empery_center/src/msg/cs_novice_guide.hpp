#ifndef EMPERY_CENTER_MSG_CS_NOVICE_GUIDE_HPP_
#define EMPERY_CENTER_MSG_CS_NOVICE_GUIDE_HPP_


#include <poseidon/cbpp/message_base.hpp>
namespace EmperyCenter
{
   namespace Msg
   {
       #define MESSAGE_NAME CS_GetTaskStepInfoReqMessage
       #define MESSAGE_ID 2000
       #define MESSAGE_FIELDS \
                           FIELD_VUINT (task_id)
       #include <poseidon/cbpp/message_generator.hpp>

       #define MESSAGE_NAME CS_GuideTaskStepReqMessage
       #define MESSAGE_ID 2001
       #define MESSAGE_FIELDS \
                           FIELD_VUINT (task_id) \
                           FIELD_VUINT (step_id)
       #include <poseidon/cbpp/message_generator.hpp>
   }
}
#endif