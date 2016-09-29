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


EVENT_LISTENER(Events::CreateWarehouseBuildingTrace, event){
	const auto log = LogClient::require();

	Msg::SL_CreateWarehouseBuildingTrace msg;
	msg.account_uuid    = event.account_uuid.str();
	msg.legion_uuid   	= event.legion_uuid.str();
	msg.x    			= event.x;
	msg.y    			= event.y;
	msg.created_time   = event.created_time;

	log->send(msg);
}


EVENT_LISTENER(Events::OpenWarehouseBuildingTrace, event){
	const auto log = LogClient::require();

	Msg::SL_OpenWarehouseBuildingTrace msg;
	msg.account_uuid    = event.account_uuid.str();
	msg.legion_uuid   	= event.legion_uuid.str();
	msg.x    			= event.x;
	msg.y    			= event.y;
	msg.level    		= event.level;
	msg.open_time   = event.open_time;

	log->send(msg);
}

EVENT_LISTENER(Events::RobWarehouseBuildingTrace, event){
	const auto log = LogClient::require();

	Msg::SL_RobWarehouseBuildingTrace msg;
	msg.account_uuid    = event.account_uuid.str();
	msg.legion_uuid   	= event.legion_uuid.str();
	msg.ntype    		= event.ntype;
	msg.amount    		= event.amount;
	msg.level    		= event.level;
	msg.rob_time        = event.rob_time;

	log->send(msg);
}



}
