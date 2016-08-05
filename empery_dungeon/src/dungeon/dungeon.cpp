#include "../precompiled.hpp"
#include "common.hpp"
#include "../../../empery_center/src/msg/sd_dungeon.hpp"
#include "../../../empery_center/src/msg/err_dungeon.hpp"
#include "../singletons/dungeon_map.hpp"
#include "../dungeon.hpp"
#include "../dungeon_object.hpp"

namespace EmperyDungeon {

DUNGEON_SERVLET(Msg::SD_DungeonCreate, dungeon, req){
	auto dungeon_uuid = DungeonUuid(req.dungeon_uuid);
	auto dungeon_type_id = DungeonTypeId(req.dungeon_type_id);
	auto founder_uuid = AccountUuid(req.founder_uuid);
	boost::shared_ptr<Dungeon> new_dunge = boost::make_shared<Dungeon>(dungeon_uuid, dungeon_type_id,dungeon,founder_uuid);
	new_dunge->set_dungeon_duration(1000*60*3);
	DungeonMap::replace_dungeon_no_synchronize(new_dunge);
	return Response();
}

DUNGEON_SERVLET(Msg::SD_DungeonDestroy, dungeon, req){
	auto dungeon_uuid = DungeonUuid(req.dungeon_uuid);
	DungeonMap::remove_dungeon_no_synchronize(dungeon_uuid);
	return Response();
}

DUNGEON_SERVLET(Msg::SD_DungeonObjectInfo, dungeon, req){
	auto dungeon_uuid = DungeonUuid(req.dungeon_uuid);
	auto expect_dungeon = DungeonMap::get(dungeon_uuid);
	if(!expect_dungeon){
		return Response(Msg::ERR_NO_SUCH_DUNGEON) << dungeon_uuid;
	}
	auto dungeon_object_uuid = DungeonObjectUuid(req.dungeon_object_uuid);
	auto dungeon_object_type_id = DungeonObjectTypeId(req.map_object_type_id);
	auto owner_uuid = AccountUuid(req.owner_uuid);
	auto coord = Coord(req.x, req.y);
	auto dungeon_object = expect_dungeon->get_object(dungeon_object_uuid);

	//属性和buff
	boost::container::flat_map<AttributeId, std::int64_t> attributes;
	attributes.reserve(req.attributes.size());
	for(auto it = req.attributes.begin(); it != req.attributes.end(); ++it){
		attributes.emplace(AttributeId(it->attribute_id), it->value);
	}

	boost::container::flat_map<BuffId, DungeonObject::BuffInfo> buffs;
	buffs.reserve(req.buffs.size());
	for(auto it = req.buffs.begin(); it != req.buffs.end(); ++it){
		DungeonObject::BuffInfo info = {};
		info.buff_id    = BuffId(it->buff_id);
		info.duration   = saturated_sub(it->time_end, it->time_begin);
		info.time_begin = it->time_begin;
		info.time_end   = it->time_end;
		buffs.emplace(BuffId(it->buff_id),std::move(info));
	}

	if(!dungeon_object){
		dungeon_object = boost::make_shared<DungeonObject>(dungeon_uuid,dungeon_object_uuid,dungeon_object_type_id,owner_uuid,coord);
		//副本中的怪物创建的时候设置下action,
		if(dungeon_object->is_monster()){
			DungeonObject::Action act = DungeonObject::ACT_GUARD;
			switch(dungeon_object->require_ai_control()->get_ai_id()){
				case DungeonObject::AI_MONSTER_AUTO_SEARCH_TARGET:
					act = DungeonObject::ACT_MONSTER_SEARCH_TARGET;
					break;
				case DungeonObject::AI_MONSTER_PATROL:
					act = DungeonObject::ACT_MONSTER_PATROL;
					break;
			}
			std::deque<std::pair<signed char, signed char>> waypoints;
			dungeon_object->set_action(coord, waypoints, act,"");
		}
	}
	dungeon_object->set_attributes(std::move(attributes));
	for(auto it = buffs.begin(); it != buffs.end(); ++it){
		dungeon_object->set_buff(it->second.buff_id, it->second.time_begin, it->second.duration);
	}
	expect_dungeon->replace_dungeon_object_no_synchronize(dungeon_object);
	return Response();
}

DUNGEON_SERVLET(Msg::SD_DungeonObjectRemoved, dungeon, req){
	auto dungeon_uuid = DungeonUuid(req.dungeon_uuid);
	auto dungeon_object_uuid = DungeonObjectUuid(req.dungeon_object_uuid);
	auto expect_dungeon = DungeonMap::get(dungeon_uuid);
	if(!expect_dungeon){
		return Response(Msg::ERR_NO_SUCH_DUNGEON) << dungeon_uuid;
	}
	expect_dungeon->reove_dungeon_object_no_synchronize(dungeon_object_uuid);
	return Response();
}


DUNGEON_SERVLET(Msg::SD_DungeonSetAction, dengeon, req){
	auto dungeon_uuid = DungeonUuid(req.dungeon_uuid);
	auto dungeon_object_uuid = DungeonObjectUuid(req.dungeon_object_uuid);
	auto expect_dungeon = DungeonMap::get(dungeon_uuid);
	if(!expect_dungeon){
		return Response(Msg::ERR_NO_SUCH_DUNGEON) << dungeon_uuid;
	}
	auto dungeon_object = expect_dungeon->get_object(dungeon_object_uuid);
	if(!dungeon_object){
		return Response(Msg::ERR_NO_SUCH_DUNGEON_OBJECT) << dungeon_object_uuid;
	}
	const auto from_coord = Coord(req.x, req.y);
	std::deque<std::pair<signed char, signed char>> waypoints;
	for(auto it = req.waypoints.begin(); it != req.waypoints.end(); ++it){
		waypoints.emplace_back(it->dx, it->dy);
	}
	dungeon_object->set_action(from_coord, std::move(waypoints), static_cast<DungeonObject::Action>(req.action), std::move(req.param));

	return Response();
}



}
