#include "../precompiled.hpp"
#include "common.hpp"
#include "../../../empery_center/src/msg/sd_dungeon.hpp"
#include "../../../empery_center/src/msg/err_dungeon.hpp"
#include "../singletons/dungeon_map.hpp"
#include "../dungeon.hpp"
#include "../dungeon_object.hpp"

namespace EmperyDungeon {

DUNGEON_SERVLET(Msg::SD_DungeonCreate, dungeon, req){
	auto dunge_uuid = DungeonUuid(req.dungeon_uuid);
	auto dungeon_type_id = DungeonTypeId(req.dungeon_type_id);
	boost::shared_ptr<Dungeon> new_dunge = boost::make_shared<Dungeon>(dunge_uuid, dungeon_type_id,dungeon,AccountUuid());
	DungeonMap::replace_dungeon_no_synchronize(new_dunge);
	return Response();
}

DUNGEON_SERVLET(Msg::SD_DungeonDestroy, dungeon, req){
	auto dunge_uuid = DungeonUuid(req.dungeon_uuid);
	DungeonMap::remove_dungeon_no_synchronize(dunge_uuid);
	return Response();
}

DUNGEON_SERVLET(Msg::SD_DungeonObjectInfo, dungeon, req){
	return Response();
}

DUNGEON_SERVLET(Msg::SD_DungeonObjectRemoved, dungeon, req){
	return Response();
}
/*
DUNGEON_SERVLET(Msg::SD_MapSetAction, dengeon, req){
	return Response();
}
*/

}
