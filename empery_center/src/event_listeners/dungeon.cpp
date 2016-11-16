#include "../precompiled.hpp"
#include "common.hpp"
#include "../events/dungeon.hpp"
#include "../msg/sl_dungeon.hpp"
#include "../singletons/log_client.hpp"

namespace EmperyCenter {

EVENT_LISTENER(Events::DungeonCreated, event){
	const auto log = LogClient::require();

	Msg::SL_DungeonCreated msg;
	msg.account_uuid    = event.account_uuid.str();
	msg.dungeon_type_id = event.dungeon_type_id.get();
	log->send(msg);
}

EVENT_LISTENER(Events::DungeonDeleted, event){
	const auto log = LogClient::require();

	Msg::SL_DungeonDeleted msg;
	msg.account_uuid    = event.account_uuid.str();
	msg.dungeon_type_id = event.dungeon_type_id.get();
	log->send(msg);
}

EVENT_LISTENER(Events::DungeonFinish, event){
	const auto log = LogClient::require();

	Msg::SL_DungeonFinish msg;
	msg.account_uuid    = event.account_uuid.str();
	msg.dungeon_type_id = event.dungeon_type_id.get();
	msg.begin_time      = event.begin_time;
	msg.finish_time     = event.finish_time;
	msg.finished        = event.finished;
	log->send(msg);
}

}
