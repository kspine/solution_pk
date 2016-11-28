#ifndef EMPERY_CENTER_MSG_SL_NOVICE_GUIDE_HPP_
#define EMPERY_CENTER_MSG_SL_NOVICE_GUIDE_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    SL_NoviceGuideTrace
#define MESSAGE_ID      58722
#define MESSAGE_FIELDS  \
    FIELD_STRING        (account_uuid)	\
    FIELD_VUINT         (task_id)       \
    FIELD_VUINT         (step_id)       \
    FIELD_VUINT         (created_time)
#include <poseidon/cbpp/message_generator.hpp>
   }
}
#endif