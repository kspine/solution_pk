#ifndef EMPERY_CENTER_MSG_CS_LEGION_PACKAGE_HPP_
#define EMPERY_CENTER_MSG_CS_LEGION_PACKAGE_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter
{
    namespace Msg
    {

        #define MESSAGE_NAME    CS_GetSharePackageInfoReqMessage
        #define MESSAGE_ID      1700
        #define MESSAGE_FIELDS
        #include <poseidon/cbpp/message_generator.hpp>

        #define MESSAGE_NAME    CS_ReceiveTaskRewardReqMessage
        #define MESSAGE_ID      1701
        #define MESSAGE_FIELDS  \
            FIELD_VUINT          (task_id) \
            FIELD_VUINT          (task_package_item_id) \
            FIELD_VUINT          (share_package_item_id)
        #include <poseidon/cbpp/message_generator.hpp>

        #define MESSAGE_NAME    CS_ReceiveSharedRewardReqMessage
        #define MESSAGE_ID      1702
        #define MESSAGE_FIELDS  \
            FIELD_STRING       (share_account_uuid) \
            FIELD_VUINT        (task_id) \
            FIELD_VUINT        (share_package_item_id)
        #include <poseidon/cbpp/message_generator.hpp>
    }
}

#endif