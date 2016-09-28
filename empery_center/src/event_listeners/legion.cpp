#include "../precompiled.hpp"
#include "common.hpp"
#include "../events/legion.hpp"
#include "../msg/sl_legion.hpp"
#include "../singletons/log_client.hpp"

namespace EmperyCenter {

EVENT_LISTENER(Events::LegionDisband, event){
	const auto log = LogClient::require();

	Msg::SL_LegionDisbandLog msg;
	msg.account_uuid    = event.account_uuid.str();
	msg.legion_name     = event.legion_name;
	msg.disband_time    = event.disband_time;
	log->send(msg);
}



}
