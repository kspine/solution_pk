#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/ds_dungeon.hpp"
#include "../msg/err_dungeon.hpp"
#include "../singletons/dungeon_map.hpp"
#include "../dungeon.hpp"
#include "../dungeon_object.hpp"
#include "../msg/err_map.hpp"

namespace EmperyCenter {

DUNGEON_SERVLET(Msg::DS_DungeonRegisterServer, server, /* req */){
	DungeonMap::add_server(server);

	return Response();
}

DUNGEON_SERVLET(Msg::DS_DungeonUpdateObjectAction, server, req){
	const auto dungeon_uuid = DungeonUuid(req.dungeon_uuid);
	const auto dungeon = DungeonMap::get(dungeon_uuid);
	if(!dungeon){
		return Response(Msg::ERR_NO_SUCH_DUNGEON) <<dungeon_uuid;
	}
	const auto test_server = dungeon->get_server();
	if(server != test_server){
		return Response(Msg::ERR_DUNGEON_SERVER_CONFLICT);
	}

	const auto dungeon_object_uuid = DungeonObjectUuid(req.dungeon_object_uuid);
	const auto dungeon_object = dungeon->get_object(dungeon_object_uuid);
	if(!dungeon_object){
		return Response(Msg::ERR_NO_SUCH_DUNGEON_OBJECT) <<dungeon_object_uuid;
	}

	// const auto old_coord = dungeon_object->get_coord();
	const auto new_coord = Coord(req.x, req.y);
	dungeon_object->set_coord_no_synchronize(new_coord); // noexcept
	dungeon_object->set_action(req.action, std::move(req.param));

	return Response();
}

DUNGEON_SERVLET(Msg::DS_DungeonObjectAttackAction, server, req){
	const auto dungeon_uuid = DungeonUuid(req.dungeon_uuid);
	const auto dungeon = DungeonMap::get(dungeon_uuid);
	if(!dungeon){
		return Response(Msg::ERR_NO_SUCH_DUNGEON) <<dungeon_uuid;
	}
	const auto test_server = dungeon->get_server();
	if(server != test_server){
		return Response(Msg::ERR_DUNGEON_SERVER_CONFLICT);
	}

	const auto attacking_object_uuid = DungeonObjectUuid(req.attacking_object_uuid);
	const auto attacked_object_uuid = DungeonObjectUuid(req.attacked_object_uuid);

	// 结算战斗伤害。
	const auto attacking_object = dungeon->get_object(attacking_object_uuid);
	if(!attacking_object || attacking_object->is_virtually_removed()){
		return Response(Msg::ERR_NO_SUCH_DUNGEON_OBJECT) <<attacking_object_uuid;
	}
	const auto attacked_object = dungeon->get_object(attacked_object_uuid);
	if(!attacked_object || attacked_object->is_virtually_removed()){
		return Response(Msg::ERR_NO_SUCH_DUNGEON_OBJECT) <<attacked_object_uuid;
	}

	const auto attacking_object_type_id = attacking_object->get_map_object_type_id();
	const auto attacking_account_uuid = attacking_object->get_owner_uuid();
	const auto attacking_coord = attacking_object->get_coord();

	const auto attacked_object_type_id = attacked_object->get_map_object_type_id();
	const auto attacked_account_uuid = attacked_object->get_owner_uuid();
	const auto attacked_coord = attacked_object->get_coord();

	if(attacking_account_uuid == attacked_account_uuid){
		return Response(Msg::ERR_CANNOT_ATTACK_FRIENDLY_OBJECTS);
	}

	const auto utc_now = Poseidon::get_utc_time();

	attacking_object->recalculate_attributes(false);
	attacked_object->recalculate_attributes(false);

	//
	(void)attacking_object_type_id;
	(void)attacking_account_uuid;
	(void)attacking_coord;
	(void)attacked_object_type_id;
	(void)attacked_account_uuid;
	(void)attacked_coord;
	(void)utc_now;


	return Response();
}

}
