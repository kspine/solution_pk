#ifndef EMPERY_CENTER_MSG_ERR_LEGION_PACKAGE_HPP_
#define EMPERY_CENTER_MSG_ERR_LEGION_PACKAGE_HPP_

#include <poseidon/cbpp/status_codes.hpp>

namespace EmperyCenter
{
    namespace Msg
     {
        using namespace Poseidon::Cbpp::StatusCodes;
        enum
        {
            ERR_DAILY_SHARE_NUMBER_INSUFFICIENT = 80000, //剩余可领分享次数不足
            ERR_LEGION_PACKAGE_NOT_EXISTS       = 80001, //军团礼包不存在
			ERR_LEGION_PACKAGE_TASK_RECEIVED    = 80002, //军团礼包任务礼包已经领取
			ERR_LEGION_PACKAGE_SHARE_RECEIVED   = 80003, //军团礼包分享礼包已经领取
            ERR_LEGION_PACKAGE_SHARE_EXPIRE     = 80004, //军团礼包分享超期
			ERR_LEGION_PACKAGE_SHARE_NOTEXPIRE  = 80005, //军团礼包分享未超期
        };
    }
}

#endif
