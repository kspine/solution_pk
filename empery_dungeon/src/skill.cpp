#include "precompiled.hpp"
#include "skill.hpp"
#include "dungeon_object.hpp"
#include "dungeon.hpp"
#include "singletons/dungeon_map.hpp"
#include "dungeon_client.hpp"
#include "cbpp_response.hpp"
#include "data/skill.hpp"
#include "../../empery_center/src/msg/ds_dungeon.hpp"
#include "../../empery_center/src/msg/err_dungeon.hpp"
#include "dungeon_utilities.hpp"

namespace EmperyDungeon {

namespace Msg = ::EmperyCenter::Msg;

namespace {
	constexpr std::array<std::array<std::pair<std::int8_t, std::int8_t>, 8>, 2>
		g_cleave_right_damage_table = {{
			{{
				{ 0, 1},{ 1, 0},{ 0, -1},
				{ 1, 2},{ 2, 1},{ 2, 0}, { 2, -1}, { 1, -2}
			}}, {{
				{ 1, 1}, { 1, 0},{ 1, -1},
				{ 1, 2}, { 2, 1}, {2, 0}, { 2,-1}, { 1, -2}
			}},
		}};

	constexpr std::array<std::array<std::pair<std::int8_t, std::int8_t>, 8>, 2>
		g_cleave_left_damage_table = {{
			{{
				{ -1, 1},{ -1, 0},{ -1, -1},
				{ -1, 2},{ -2, 1},{ -2, 0}, { -2, -1}, { -1, -2}
			}}, {{
				{ 0, 1}, { -1, 0},{ 0, -1},
				{ -1, 2}, { -1, 1}, {-2, 0}, { -1,-1}, { -1, -2}
			}},
		}};

	constexpr std::array<std::array<std::pair<std::int8_t, std::int8_t>, 8>, 2>
		g_cleave_up_left_damage_table = {{
			{{
				{ -1, 0},{ -1, 1},{ 0, 1},
				{ -2, 0},{ -2, 1},{ -1, 2}, { 0, 2}, { 1, 2}
			}}, {{
				{ -1, 0}, { 0, 1},{ 1, 1},
				{ -2, 0}, { -1, 1}, {-1, 2}, { 0,2}, { 1, 2}
			}},
		}};

	constexpr std::array<std::array<std::pair<std::int8_t, std::int8_t>, 8>, 2>
		g_cleave_up_right_damage_table = {{
			{{
				{ -1, 1},{ 0, 1},{ 1, 0},
				{ -1, 2},{ 0, 2},{ 1, 2}, { 1, 1}, { 2, 0}
			}}, {{
				{ 0, 1}, { 1, 1},{ 1, 0},
				{ -1, 2}, { 0, 2}, {1, 2}, { 2,1}, { 2, 0}
			}},
		}};

	constexpr std::array<std::array<std::pair<std::int8_t, std::int8_t>, 8>, 2>
		g_cleave_down_left_table = {{
			{{
				{ -1, 0},{ -1, -1},{ 0, -1},
				{ -2, 0},{ -2, -1},{ -1, -2}, { 0, -2}, { 1, -2}
			}}, {{
				{ -1, 0}, { 0, -1},{ 1, -1},
				{ -2, 0}, { -1, -1}, {-1, -2}, { 0,-2}, { 1, -2}
			}},
		}};

	constexpr std::array<std::array<std::pair<std::int8_t, std::int8_t>, 8>, 2>
		g_cleave_down_right_table = {{
			{{
				{ -1, -1},{ 0, -1},{ 1, 0},
				{ -1, -2},{ 0, -2},{ 1, -2}, { 1, -1}, { 2, 0}
			}}, {{
				{ 0, -1}, { 1, -1},{ 1, 0},
				{ -1, -2}, { 0, -2}, {1, -2}, { 2,-1}, { 2, 0}
			}},
		}};

	void get_cleave_skill_effect_range(std::vector<Coord> &ret, Coord origin,Skill::Skill_Direct direct){
		PROFILE_ME;

		const bool in_odd_row = origin.y() & 1;
		if(direct != Skill::DIRECT_RIGHT && direct != Skill::DIRECT_LEFT &&
			direct != Skill::DIRECT_UP_LEFT && direct != Skill::DIRECT_UP_RIGHT &&
			direct != Skill::DIRECT_DOWN_LEFT && direct != Skill::DIRECT_DOWN_RIGHT){
			LOG_EMPERY_DUNGEON_WARNING("get_cleave_skill_effect_range,ERROR direct", direct);
			return;
		}
		std::array<std::pair<std::int8_t, std::int8_t>, 8> table;

		if(direct == Skill::DIRECT_RIGHT){
			table = g_cleave_right_damage_table.at(in_odd_row);
		}else if(direct == Skill::DIRECT_LEFT){
			table = g_cleave_left_damage_table.at(in_odd_row);
		}else if(direct == Skill::DIRECT_UP_LEFT){
			table = g_cleave_up_left_damage_table.at(in_odd_row);
		}else if(direct == Skill::DIRECT_UP_RIGHT ){
			table = g_cleave_up_right_damage_table.at(in_odd_row);
		}else if(direct == Skill::DIRECT_DOWN_LEFT){
			table = g_cleave_down_left_table.at(in_odd_row);
		}else if(direct == Skill::DIRECT_DOWN_RIGHT ){
			table = g_cleave_down_right_table.at(in_odd_row);
		}

		auto begin = table.begin();

		ret.reserve(ret.size() + static_cast<std::size_t>(table.end() - begin));
		for(auto it = begin; it != table.end(); ++it){
			ret.emplace_back(origin.x() + it->first, origin.y() + it->second);
		}
	}
}

Skill::Skill(DungeonMonsterSkillId skill_id,const boost::weak_ptr<DungeonObject> owner)
	: m_skill_id(skill_id),m_owner(owner),m_next_execute_time(0),m_cast_coord(Coord(0,0))
{
}
Skill::~Skill(){
}

void Skill::set_next_execute_time(std::uint64_t next_execute_time){
	m_next_execute_time = next_execute_time;
}

void Skill::set_cast_coord(Coord coord){
	m_cast_coord  = coord;
}

Skill::Skill_Direct Skill::get_cast_direct(){
	return DIRECT_NONE;
}

void  Skill::do_effects(){
	std::vector<Coord> coords;
	notify_effects(coords);
}

void   Skill::notify_effects(const std::vector<Coord> &coords){
	auto owner_object  = get_owner();
	if(!owner_object){
		LOG_EMPERY_DUNGEON_WARNING("skill do damge,owner object is null");
		return;
	}
	const auto dungeon_uuid = owner_object->get_dungeon_uuid();
	const auto dungeon = DungeonMap::require(dungeon_uuid);
	const auto dungeon_client = dungeon->get_dungeon_client();
	if(!dungeon_client){
		LOG_EMPERY_DUNGEON_WARNING("skill do damge, dungeon client is null");
		return;
	}
	try{
		Msg::DS_DungeonObjectSkillEffect msg;
		msg.dungeon_uuid             = owner_object->get_dungeon_uuid().str();
		msg.attacking_account_uuid   = owner_object->get_owner_uuid().str();
		msg.attacking_object_uuid    = owner_object->get_dungeon_object_uuid().str();
		msg.attacking_object_type_id = owner_object->get_dungeon_object_type_id().get();
		msg.attacking_coord_x        = owner_object->get_coord().x();
		msg.attacking_coord_y        = owner_object->get_coord().y();
		msg.attacked_coord_x         = m_cast_coord.x();
		msg.attacked_coord_y         = m_cast_coord.y();
		msg.skill_type_id            = get_skill_id().get();
		for(auto it = coords.begin(); it != coords.end(); ++it){
				auto &temp_coord  = *it;
				auto &range_coord = *msg.effect_range.emplace(msg.effect_range.end());
				range_coord.x = temp_coord.x();
				range_coord.y = temp_coord.y();
		}
		LOG_EMPERY_DUNGEON_FATAL(msg);
		auto sresult = dungeon_client->send_and_wait(msg);
		if(sresult.first != Msg::ST_OK){
			LOG_EMPERY_DUNGEON_DEBUG("Center server returned an error when report skill effect: code = ", sresult.first, ", msg = ", sresult.second);
		}
	} catch(std::exception &e){
		LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
		dungeon_client->shutdown(e.what());
	}
}

void Skill::do_damage(const std::vector<Coord> &coords){
	auto owner_object  = get_owner();
	if(!owner_object){
		LOG_EMPERY_DUNGEON_WARNING("skill do damge,owner object is null");
		return;
	}
	const auto dungeon_uuid = owner_object->get_dungeon_uuid();
	const auto dungeon = DungeonMap::require(dungeon_uuid);
	const auto dungeon_client = dungeon->get_dungeon_client();
	if(!dungeon_client){
		LOG_EMPERY_DUNGEON_WARNING("skill do damge, dungeon client is null");
		return;
	}
	auto skill_data  = Data::Skill::require(get_skill_id());
	auto attack_rate = skill_data->attack_rate;
	auto attack_fix  = skill_data->attack_fix;
	auto attack_type = skill_data->attack_type;
	double k = 0.06;
	auto total_attack = owner_object->get_total_attack()*attack_rate + attack_fix;
	for(auto it = coords.begin(); it != coords.end(); ++it){
		auto &coord = *it;
		auto target_object = dungeon->get_object(coord);
		if(target_object){
			if(target_object->get_owner_uuid() != owner_object->get_owner_uuid()){
				try {
						//伤害计算
						double relative_rate = Data::DungeonObjectRelative::get_relative(attack_type,target_object->get_arm_defence_type());
						double total_defense = target_object->get_total_defense();
						double damage = total_attack * 1 * relative_rate * (1 - k *total_defense/(1 + k*total_defense));
						Msg::DS_DungeonObjectAttackAction msg;
						msg.dungeon_uuid           = owner_object->get_dungeon_uuid().str();
						/*
						msg.attacking_account_uuid = get_owner_uuid().str();
						msg.attacking_object_uuid = get_dungeon_object_uuid().str();
						msg.attacking_object_type_id = get_dungeon_object_type_id().get();
						msg.attacking_coord_x = coord.x();
						msg.attacking_coord_y = coord.y();
						*/
						msg.attacked_account_uuid = target_object->get_owner_uuid().str();
						msg.attacked_object_uuid  = target_object->get_dungeon_object_uuid().str();
						msg.attacked_object_type_id = target_object->get_dungeon_object_type_id().get();
						msg.attacked_coord_x = target_object->get_coord().x();
						msg.attacked_coord_y = target_object->get_coord().y();
						msg.result_type = 1;
						msg.soldiers_damaged = damage;
						LOG_EMPERY_DUNGEON_FATAL(msg);
						auto sresult = dungeon_client->send_and_wait(msg);
						if(sresult.first != Poseidon::Cbpp::ST_OK){
							LOG_EMPERY_DUNGEON_DEBUG("skill cleave damge fail", sresult.first, ", msg = ", sresult.second);
							continue;
						}
					} catch(std::exception &e){
						LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
						dungeon_client->shutdown(e.what());
					}
			}
		}
	}
}

SkillCleave::SkillCleave(DungeonMonsterSkillId skill_id,const boost::weak_ptr<DungeonObject> owner)
	:Skill(skill_id,owner){
}

SkillCleave::~SkillCleave(){
}

Skill::Skill_Direct SkillCleave::get_cast_direct(){
	if(m_cast_coord == Coord(0,0)){
		return DIRECT_NONE;
	}
	auto owner_object  = get_owner();
	if(!owner_object){
		LOG_EMPERY_DUNGEON_WARNING("CANNT GET SKILL OWNER, skill_id = ",get_skill_id());
		return DIRECT_NONE;
	}
	auto origin_coord = owner_object->get_coord();
	auto distance = get_distance_of_coords(m_cast_coord,origin_coord);
	if(distance > 1){
		return DIRECT_NONE;
	}
	if(origin_coord.x() + 1 == m_cast_coord.x()){
		return DIRECT_RIGHT;
	}
	if(m_cast_coord.x() + 1 == origin_coord.x()){
		return DIRECT_LEFT;
	}
	const bool in_odd_row = origin_coord.y() & 1;
	//奇数行
	if(in_odd_row){
		if((origin_coord.x() == m_cast_coord.x()) &&
			(origin_coord.y() + 1 == m_cast_coord.y()) ){
			return DIRECT_UP_LEFT;
		}
		if((origin_coord.x() + 1 == m_cast_coord.x()) &&
			(origin_coord.y() + 1 == m_cast_coord.y()) ){
			return DIRECT_UP_RIGHT;
		}
		if((origin_coord.x()  == m_cast_coord.x()) &&
			(origin_coord.y() -1 == m_cast_coord.y()) ){
			return DIRECT_DOWN_LEFT;
		}
		if((origin_coord.x() + 1 == m_cast_coord.x()) &&
			(origin_coord.y() - 1 == m_cast_coord.y()) ){
			return DIRECT_DOWN_RIGHT;
		}
	}else{
		if((origin_coord.x() - 1 == m_cast_coord.x()) &&
			(origin_coord.y() + 1 == m_cast_coord.y()) ){
			return DIRECT_UP_LEFT;
		}
		if((origin_coord.x()  == m_cast_coord.x()) &&
			(origin_coord.y() + 1 == m_cast_coord.y()) ){
			return DIRECT_UP_RIGHT;
		}
		if((origin_coord.x() -1 == m_cast_coord.x()) &&
			(origin_coord.y() -1 == m_cast_coord.y()) ){
			return DIRECT_DOWN_LEFT;
		}
		if((origin_coord.x() == m_cast_coord.x()) &&
			(origin_coord.y() - 1 == m_cast_coord.y()) ){
			return DIRECT_DOWN_RIGHT;
		}
	}
	return DIRECT_NONE;
}

void  SkillCleave::do_effects(){
	auto owner_object  = get_owner();
	if(!owner_object){
		LOG_EMPERY_DUNGEON_WARNING("CANNT GET SKILL OWNER, skill_id = ",get_skill_id());
		return;
	}
	auto origin_coord = owner_object->get_coord();
	auto direct = get_cast_direct();
	std::vector<Coord> coords;
	get_cleave_skill_effect_range(coords,origin_coord,direct);
	Skill::notify_effects(coords);
	Skill::do_damage(coords);
}

SkillCyclone::SkillCyclone(DungeonMonsterSkillId skill_id,const boost::weak_ptr<DungeonObject> owner)
	:Skill(skill_id,owner){
}

SkillCyclone::~SkillCyclone(){
}

void  SkillCyclone::do_effects(){
	auto owner_object  = get_owner();
	if(!owner_object){
		LOG_EMPERY_DUNGEON_WARNING("CANNT GET SKILL OWNER, skill_id = ",get_skill_id());
		return;
	}
	auto origin_coord = owner_object->get_coord();
	auto skill_data  = Data::Skill::require(get_skill_id());
	auto range = skill_data->skill_range;
	std::vector<Coord> coords;
	for(unsigned i = 1; i <= range; i++){
		get_surrounding_coords(coords,origin_coord,i);
	}
	Skill::notify_effects(coords);
	Skill::do_damage(coords);
}

}
