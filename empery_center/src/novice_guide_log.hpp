#ifndef EMPERY_CENTER_NOVICE_GUIDE_LOG_HPP_
#define EMPERY_CENTER_NOVICE_GUIDE_LOG_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/fwd.hpp>

#include "id_types.hpp"


namespace EmperyCenter{

     class NoviceGuideLog{

      public: 
        NoviceGuideLog();
        ~NoviceGuideLog();
      public:
            static void NoviceGuideTrace(AccountUuid account_uuid, TaskId task_id,std::uint64_t step_id,std::uint64_t created_time);
    };
}

#endif