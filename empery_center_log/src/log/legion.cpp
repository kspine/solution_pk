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



LOG_SERVLET(Msg::SL_LegionMemberTrace, log, req){
	auto obj = boost::make_shared<MySql::CenterLog_LegionMemberTrace>(
			Poseidon::Uuid(req.account_uuid),
			Poseidon::Uuid(req.legion_uuid),
			Poseidon::Uuid(req.action_uuid),
			req.action_type,
			req.created_time);
		obj->async_save(false, true);
}

LOG_SERVLET(Msg::SL_LeagueLegionTrace, log, req){
	auto obj = boost::make_shared<MySql::CenterLog_LeagueLegionTrace>(
		Poseidon::Uuid(req.legion_uuid),
		Poseidon::Uuid(req.league_uuid),
		Poseidon::Uuid(req.action_uuid),
		req.action_type,
		req.created_time);
	obj->async_save(false, true);
}

LOG_SERVLET(Msg::SL_LegionPersonalDonateChanged, log, req){
	auto obj = boost::make_shared<MySql::CenterLog_PersonalDonateChanged>(
		Poseidon::get_utc_time(),
		Poseidon::Uuid(req.account_uuid), req.item_id, req.old_count, req.new_count,
		static_cast<std::int64_t>(req.new_count - req.old_count), req.reason, req.param1, req.param2, req.param3);
	obj->async_save(false, true);
}

LOG_SERVLET(Msg::SL_LegionMoneyChanged, log, req){
	auto obj = boost::make_shared<MySql::CenterLog_LegionMoneyChanged>(
		Poseidon::get_utc_time(),
		Poseidon::Uuid(req.legion_uuid), req.item_id, req.old_count, req.new_count,
		static_cast<std::int64_t>(req.new_count - req.old_count), req.reason, req.param1, req.param2, req.param3);
	obj->async_save(false, true);
}
}
