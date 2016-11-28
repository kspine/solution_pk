#include "precompiled.hpp"
#include "mmain.hpp"
#include "novice_guide_log.hpp"
#include "events/novice_guide.hpp"


namespace EmperyCenter{



NoviceGuideLog::NoviceGuideLog()
{
}
NoviceGuideLog::~NoviceGuideLog()
{
}

  void NoviceGuideLog::NoviceGuideTrace(AccountUuid account_uuid, TaskId task_id, std::uint64_t step_id,std::uint64_t created_time)
  {
     LOG_EMPERY_CENTER_FATAL("_______________________NoviceGuideLog::Inner______________________");

     const auto event = boost::make_shared<Events::NoviceGuideTrace>(account_uuid,task_id,step_id,created_time);
     const auto withdrawn = boost::make_shared<bool>(true);
     Poseidon::async_raise_event(event,withdrawn);
     *withdrawn = false;
  }
}