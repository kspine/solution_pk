#include "../precompiled.hpp"
#include "common.hpp"
#include "../../../empery_center/src/msg/sd_dungeon.hpp"
#include "../../../empery_center/src/msg/err_dungeon.hpp"
#include "../singletons/dungeon_map.hpp"
#include "../dungeon.hpp"
#include "../dungeon_object.hpp"
#include "../trigger.hpp"
#include "../../../empery_center/src/attribute_ids.hpp"
#include "../checked_arithmetic.hpp"
#include "../dungeon_buff.hpp"

namespace EmperyDungeon {

DUNGEON_SERVLET(Msg::SD_DungeonCreate, dungeon, req){
	auto dungeon_uuid = DungeonUuid(req.dungeon_uuid);
	auto dungeon_type_id = DungeonTypeId(req.dungeon_type_id);
	auto founder_uuid = AccountUuid(req.founder_uuid);
	auto finish_count = req.finish_count;
	auto expiry_time  = req.expiry_time;
	const auto utc_now = Poseidon::get_utc_time();
	boost::shared_ptr<Dungeon> new_dungeon = boost::make_shared<Dungeon>(dungeon_uuid, dungeon_type_id,dungeon,founder_uuid,utc_now,finish_count,expiry_time);
	LOG_EMPERY_DUNGEON_DEBUG("create dungeon ,msg = ",req);
	DungeonMap::replace_dungeon_no_synchronize(new_dungeon);
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
	auto tag = req.tag;
	auto dungeon_object = expect_dungeon->get_object(dungeon_object_uuid);

	//属性和buff
	boost::container::flat_map<AttributeId, std::int64_t> attributes;
	attributes.reserve(req.attributes.size());
	for(auto it = req.attributes.begin(); it != req.attributes.end(); ++it){
		attributes.emplace(AttributeId(it->attribute_id), it->value);
		LOG_EMPERY_DUNGEON_DEBUG("dungeon object info, dungoen_object_uuid =  ",dungeon_object_uuid," attid = ",it->attribute_id, " value = ",it->value);
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
		dungeon_object = boost::make_shared<DungeonObject>(dungeon_uuid,dungeon_object_uuid,dungeon_object_type_id,owner_uuid,coord,tag);
		//副本中的怪物创建的时候设置下action,
		if(dungeon_object->is_monster()){
			DungeonObject::Action act = DungeonObject::ACT_GUARD;
			const auto ai_id = dungeon_object->require_ai_control()->get_ai_id();
			const auto ai_data = Data::DungeonObjectAi::require(ai_id);
			switch(ai_data->ai_type){
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
	const auto old_hp = static_cast<std::uint64_t>(dungeon_object->get_attribute(EmperyCenter::AttributeIds::ID_HP_TOTAL));
	const auto old_solider_count = static_cast<std::uint64_t>(dungeon_object->get_attribute(EmperyCenter::AttributeIds::ID_SOLDIER_COUNT));
	dungeon_object->set_attributes(std::move(attributes));
	const auto new_hp = static_cast<std::uint64_t>(dungeon_object->get_attribute(EmperyCenter::AttributeIds::ID_HP_TOTAL));
	const auto new_solider_count = static_cast<std::uint64_t>(dungeon_object->get_attribute(EmperyCenter::AttributeIds::ID_SOLDIER_COUNT));
	for(auto it = buffs.begin(); it != buffs.end(); ++it){
		dungeon_object->set_buff(it->second.buff_id, it->second.time_begin, it->second.duration);
	}
	expect_dungeon->replace_dungeon_object_no_synchronize(dungeon_object);
	if(old_hp > new_hp){
		const auto dungeon_object_type_data = dungeon_object->get_dungeon_object_type_data();
		const auto hp_total = checked_mul(dungeon_object_type_data->max_soldier_count, dungeon_object_type_data->hp);
		expect_dungeon->check_triggers_hp(dungeon_object->get_tag(),hp_total,old_hp,new_hp);
		expect_dungeon->check_triggers_tag_die();
	}
	if(old_solider_count > new_solider_count){
		const auto damaged_solider_count = old_solider_count - new_solider_count;
		if(!dungeon_object->is_monster()&&(damaged_solider_count > 0)){
			expect_dungeon->update_damage_solider(dungeon_object->get_dungeon_object_type_id(),damaged_solider_count);
		}
	}
	return Response();
}

DUNGEON_SERVLET(Msg::SD_DungeonObjectRemoved, dungeon, req){
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
	const auto old_hp = static_cast<std::uint64_t>(dungeon_object->get_attribute(EmperyCenter::AttributeIds::ID_HP_TOTAL));
	const auto dungeon_object_type_data = dungeon_object->get_dungeon_object_type_data();
	const auto hp_total = checked_mul(dungeon_object_type_data->max_soldier_count, dungeon_object_type_data->hp);
	expect_dungeon->check_triggers_hp(dungeon_object->get_tag(),hp_total,old_hp,0);
	expect_dungeon->remove_dungeon_object_no_synchronize(dungeon_object_uuid);
	expect_dungeon->check_triggers_all_die();
	expect_dungeon->check_triggers_tag_die();
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

DUNGEON_SERVLET(Msg::SD_DungeonPlayerConfirmation, dengeon, req){
	auto dungeon_uuid = DungeonUuid(req.dungeon_uuid);
	auto context      = req.context;
	auto expect_dungeon = DungeonMap::get(dungeon_uuid);
	if(!expect_dungeon){
		return Response(Msg::ERR_NO_SUCH_DUNGEON) << dungeon_uuid;
	}
	expect_dungeon->on_triggers_dungeon_player_confirmation(context);
	return Response();
}

DUNGEON_SERVLET(Msg::SD_DungeonBuffInfo, dungeon, req){
	auto dungeon_uuid = DungeonUuid(req.dungeon_uuid);
	auto expect_dungeon = DungeonMap::get(dungeon_uuid);
	if(!expect_dungeon){
		return Response(Msg::ERR_NO_SUCH_DUNGEON) << dungeon_uuid;
	}
	auto coord = Coord(req.x, req.y);
	auto buff_type_id = DungeonBuffTypeId(req.buff_type_id);
	auto dungeon_buff = expect_dungeon->get_dungeon_buff(coord);
	auto expired_time = req.expired_time;
	auto create_uuid  = DungeonObjectUuid(req.create_uuid);
	auto create_owner_uuid = AccountUuid(req.create_owner_uuid);

	if(!dungeon_buff){
		dungeon_buff = boost::make_shared<DungeonBuff>(dungeon_uuid,buff_type_id,create_uuid,create_owner_uuid,coord,expired_time);
	}
	expect_dungeon->replace_dungeon_buff_no_synchronize(dungeon_buff);
	return Response();
}

DUNGEON_SERVLET(Msg::SD_DungeonBuffRemoved, dungeon, req){
	auto dungeon_uuid = DungeonUuid(req.dungeon_uuid);
	auto coord = Coord(req.x, req.y);
	auto expect_dungeon = DungeonMap::get(dungeon_uuid);
	if(!expect_dungeon){
		return Response(Msg::ERR_NO_SUCH_DUNGEON) << dungeon_uuid;
	}
	auto dungeon_buff = expect_dungeon->get_dungeon_buff(coord);
	if(!dungeon_buff){
		return Response(Msg::ERR_NO_DUNGEON_BUFF_IN_POS) << coord;
	}
	expect_dungeon->remove_dungeon_buff_no_synchronize(coord);
	return Response();
}

DUNGEON_SERVLET(Msg::SD_DungeonBegin, dengeon, req){
	auto dungeon_uuid = DungeonUuid(req.dungeon_uuid);
	auto expect_dungeon = DungeonMap::get(dungeon_uuid);
	if(!expect_dungeon){
		return Response(Msg::ERR_NO_SUCH_DUNGEON) << dungeon_uuid;
	}
	expect_dungeon->init_triggers();
	expect_dungeon->check_triggers_enter_dungeon();
	return Response();
}


}
