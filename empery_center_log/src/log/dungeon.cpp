#include "../precompiled.hpp"
#include "common.hpp"
#include "../../../empery_center/src/msg/sl_dungeon.hpp"
#include "../mysql/dungeon_created.hpp"
#include "../mysql/dungeon_deleted.hpp"
#include "../mysql/dungeon_finish.hpp"

namespace EmperyCenterLog {

LOG_SERVLET(Msg::SL_DungeonCreated, log, req){
	auto obj = boost::make_shared<MySql::CenterLog_DungeonCreated>(
		Poseidon::get_utc_time(),
		Poseidon::Uuid(req.account_uuid), req.dungeon_type_id);
	obj->async_save(false, true);
}

LOG_SERVLET(Msg::SL_DungeonDeleted, log, req){
	auto obj = boost::make_shared<MySql::CenterLog_DungeonDeleted>(
		Poseidon::get_utc_time(),
		Poseidon::Uuid(req.account_uuid), req.dungeon_type_id);
	obj->async_save(false, true);
}

LOG_SERVLET(Msg::SL_DungeonFinish, log, req){
	auto obj = boost::make_shared<MySql::CenterLog_DungeonFinish>(
		Poseidon::get_utc_time(),
		Poseidon::Uuid(req.account_uuid), req.dungeon_type_id,req.begin_time,req.finish_time, req.finished);
	obj->async_save(false, true);
}

}
