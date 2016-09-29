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

EVENT_LISTENER(Events::LeagueDisband, event){
	const auto log = LogClient::require();

	Msg::SL_LeagueDisbandLog msg;
	msg.account_uuid    = event.account_uuid.str();
	msg.league_name     = event.league_name;
	msg.disband_time    = event.disband_time;
	log->send(msg);
}


EVENT_LISTENER(Events::LegionMemberTrace, event){
	const auto log = LogClient::require();

	Msg::SL_LegionMemberTrace msg;
	msg.account_uuid   = event.account_uuid.str();
	msg.legion_uuid    = event.legion_uuid.str();
	msg.action_uuid    = event.action_uuid.str();
	msg.action_type    = event.action_type;
	msg.created_time   = event.created_time;
	log->send(msg);
}

EVENT_LISTENER(Events::LeagueLegionTrace, event){
	const auto log = LogClient::require();

	Msg::SL_LeagueLegionTrace msg;
	msg.legion_uuid    = event.legion_uuid.str();
	msg.league_uuid   = event.league_uuid.str();
	msg.action_uuid    = event.action_uuid.str();
	msg.action_type    = event.action_type;
	msg.created_time   = event.created_time;
	log->send(msg);
}



}
