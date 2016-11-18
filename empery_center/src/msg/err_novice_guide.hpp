#ifndef EMPERY_CENTER_MSG_ERR_NOVICE_GUIDE_HPP_
#define EMPERY_CENTER_MSG_ERR_NOVICE_GUIDE_HPP_


#include <poseidon/cbpp/status_codes.hpp>

namespace EmperyCenter{
   namespace Msg{
       using namespace Poseidon::Cbpp::StatusCodes;
       enum{
          ERR_CHECK_NOVICE_GUIDE_TASK_STEPID = 80006,    //新手引导任务步点校验错误
          ERR_CHECK_NOVICE_GUIDE_TASK_FINISHED = 80007,  //新手任务已经完成
          ERR_CHECK_NOVICE_GUIDE_TASK_STEPID_NOTEXIST = 80008, //新手引导任务步点不存在
       };
   }
}
#endif
