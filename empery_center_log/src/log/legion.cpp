#include "../precompiled.hpp"
#include "common.hpp"
#include "../../../empery_center/src/msg/sl_legion.hpp"
#include "../mysql/legion.hpp"


namespace EmperyCenterLog {

LOG_SERVLET(Msg::SL_LegionDisbandLog, log, req){
	auto obj = boost::make_shared<MySql::CenterLog_LegionDisband>(
		Poseidon::Uuid(req.account_uuid), req.legion_name,req.disband_time);
	obj->async_save(false, true);
}

LOG_SERVLET(Msg::SL_LeagueDisbandLog, log, req){
	auto obj = boost::make_shared<MySql::CenterLog_LeagueDisband>(
		Poseidon::Uuid(req.account_uuid), req.league_name,req.disband_time);
	obj->async_save(false, true);
}
}
