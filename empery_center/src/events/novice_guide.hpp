#ifndef EMPERY_CENTER_EVENTS_NOVICE_GUIDE_HPP_
#define EMPERY_CENTER_EVENTS_NOVICE_GUIDE_HPP_

#include <poseidon/event_base.hpp>
#include "../id_types.hpp"

namespace EmperyCenter{

 namespace Events{

    struct NoviceGuideTrace : public Poseidon::EventBase<340722> {
		AccountUuid account_uuid;
		TaskId task_id;
		std::uint64_t step_id;
		std::uint64_t created_time;

		NoviceGuideTrace(
		        AccountUuid account_uuid_,
		        TaskId task_id_,
		        std::uint64_t step_id_,
		        std::uint64_t created_time_)
		:account_uuid(account_uuid_),
			  task_id(task_id_),
			  step_id(step_id_),
			  created_time(created_time_)
        {
        }
     };
  }
}
#endif