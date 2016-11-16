#ifndef EMPERY_CENTER_MSG_SC_LEGION_FINANCIAL_HPP_
#define EMPERY_CENTER_MSG_SC_LEGION_FINANCIAL_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter{
 namespace Msg{
#define MESSAGE_NAME    SC_GetLegionFinancialInfoReqMessage
#define MESSAGE_ID      2150
#define MESSAGE_FIELDS  \
            FIELD_ARRAY(legion_financial_array,    \
            FIELD_STRING        (legion_uuid) \
            FIELD_STRING        (account_uuid)  \
            FIELD_VUINT         (item_id) \
            FIELD_VUINT         (old_count) \
            FIELD_VUINT         (new_count) \
            FIELD_VUINT         (action_id) \
            FIELD_VUINT         (action_count) \
            FIELD_VUINT         (created_time) \
         )
#include <poseidon/cbpp/message_generator.hpp>
 }
}

#endif//