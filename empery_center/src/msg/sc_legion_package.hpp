#ifndef EMPERY_CENTER_MSG_SC_LEGION_PACKAGE_HPP_
#define EMPERY_CENTER_MSG_SC_LEGION_PACKAGE_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter
{
    namespace Msg
    {
        #define MESSAGE_NAME    SC_GetSharePackageInfoReqMessage
        #define MESSAGE_ID      1750
        #define MESSAGE_FIELDS  \
            FIELD_VUINT         (share_package_taken_counts) \
            FIELD_ARRAY(share_package_array,	\
                FIELD_STRING        (account_uuid)	\
                FIELD_VUINT         (task_id) \
                FIELD_VUINT         (share_package_item_id) \
                FIELD_VUINT         (share_package_time) \
                FIELD_VUINT         (share_package_status) \
            )

#include <poseidon/cbpp/message_generator.hpp>

        #define MESSAGE_NAME    SC_ShareSyncMessage
        #define MESSAGE_ID      1751
        #define MESSAGE_FIELDS  \
          FIELD_STRING         (account_uuid)	\
          FIELD_VUINT          (task_id) \
          FIELD_VUINT          (share_package_item_id) \
          FIELD_VUINT          (share_package_time)\
          FIELD_VUINT          (share_package_taken_counts)

#include <poseidon/cbpp/message_generator.hpp>
    }
}
#endif//