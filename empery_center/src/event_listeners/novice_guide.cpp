#include "../precompiled.hpp"
#include "common.hpp"
#include "../events/novice_guide.hpp"
#include "../msg/sl_novice_guide.hpp"
#include "../singletons/log_client.hpp"

namespace EmperyCenter{

 EVENT_LISTENER(Events::NoviceGuideTrace, event){
	const auto log = LogClient::require();
	Msg::SL_NoviceGuideTrace  msg;
	msg.account_uuid   = event.account_uuid.str();
	msg.task_id        = event.task_id.get();
	msg.step_id        = event.step_id;
	msg.created_time   = event.created_time;
	log->send(msg);
  }
}