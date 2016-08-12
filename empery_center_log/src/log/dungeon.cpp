#include "../precompiled.hpp"
#include "common.hpp"
#include "../../../empery_center/src/msg/sl_dungeon.hpp"
#include "../mysql/dungeon_created.hpp"
#include "../mysql/dungeon_deleted.hpp"

namespace EmperyCenterLog {

LOG_SERVLET(Msg::SL_DungeonCreated, log, req){
	auto obj = boost::make_shared<MySql::CenterLog_DungeonCreated>(
		Poseidon::get_utc_time(),
		Poseidon::Uuid(req.account_uuid), req.dungeon_type_id);
	obj->async_save(true);
}

LOG_SERVLET(Msg::SL_DungeonDeleted, log, req){
	auto obj = boost::make_shared<MySql::CenterLog_DungeonDeleted>(
		Poseidon::get_utc_time(),
		Poseidon::Uuid(req.account_uuid), req.dungeon_type_id, req.finished);
	obj->async_save(true);
}

}
