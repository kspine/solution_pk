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
#include "data/dungeon_buff.hpp"
#include "dungeon_buff.hpp"

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


SkillRecycleDamage::SkillRecycleDamage(Coord coord_, DungeonObjectUuid dungon_objec_uuid_, AccountUuid owner_account_uuid_,std::uint64_t attack_,DungeonMonsterSkillId skill_id_, std::uint64_t next_damage_time_,std::uint64_t interval_,std::uint64_t times_,std::vector<Coord> damage_range_)
:origin_coord(coord_),dungon_objec_uuid(dungon_objec_uuid_),owner_account_uuid(owner_account_uuid_),attack(attack_),skill_id(skill_id_),next_damage_time(next_damage_time_),interval(interval_),times(times_),damage_range(damage_range_)
{
}

SkillRecycleDamage::~SkillRecycleDamage(){
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

void Skill::set_cast_params(std::string params){
	m_cast_params = params;
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
	PROFILE_ME;

	auto cast_coord = get_cast_coord();
	if(cast_coord == Coord(0,0)){
		LOG_EMPERY_DUNGEON_WARNING("skill fire can't get a valid cast target ,cast target in dungeon monster skill is 0 ?");
	}
	auto owner = get_owner();
	if(!owner){
		LOG_EMPERY_DUNGEON_WARNING("skill owner is null ?");
		return;
	}
	const auto dungeon_uuid = owner->get_dungeon_uuid();
	const auto dungeon = DungeonMap::require(dungeon_uuid);
	auto dungon_objec_uuid = owner->get_dungeon_object_uuid();
	auto account_uuid      = owner->get_owner_uuid();
	auto attack            = owner->get_total_attack();
	auto skill_id          = get_skill_id();
	auto utc_now           = Poseidon::get_utc_time();
	auto range_coords      = coords;
	auto skill_recycle_damage = boost::make_shared<SkillRecycleDamage>(cast_coord,dungon_objec_uuid,account_uuid,attack,skill_id,utc_now,0,1,std::move(range_coords));;
	if(skill_recycle_damage){
		dungeon->insert_skill_damage(skill_recycle_damage);
		dungeon->pump_skill_damage();
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
	PROFILE_ME;

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

SkillFireBrand::SkillFireBrand(DungeonMonsterSkillId skill_id,const boost::weak_ptr<DungeonObject> owner)
	:Skill(skill_id,owner){
}

SkillFireBrand::~SkillFireBrand(){
}

void  SkillFireBrand::do_effects(){
	PROFILE_ME;

	auto owner_object  = get_owner();
	if(!owner_object){
		LOG_EMPERY_DUNGEON_WARNING("CANNT GET SKILL OWNER, skill_id = ",get_skill_id());
		return;
	}
	auto cast_coord = get_cast_coord();
	if(cast_coord == Coord(0,0)){
		LOG_EMPERY_DUNGEON_WARNING("Fire brand cast coord is (0,0)");
		return;
	}
	const auto dungeon_uuid = owner_object->get_dungeon_uuid();
	const auto dungeon = DungeonMap::require(dungeon_uuid);
	std::vector<Coord> coords;
	coords.push_back(cast_coord);
	Skill::notify_effects(coords);
	do_damage(coords);
}

void   SkillFireBrand::do_damage(const std::vector<Coord> &coords){
	auto cast_coord = get_cast_coord();
	if(cast_coord == Coord(0,0)){
		LOG_EMPERY_DUNGEON_WARNING("skill fire can't get a valid cast target ,cast target in dungeon monster skill is 0 ?");
	}
	auto owner_object = get_owner();
	if(!owner_object){
		LOG_EMPERY_DUNGEON_WARNING("skill owner_object is null ?");
		return;
	}
	const auto dungeon_uuid = owner_object->get_dungeon_uuid();
	const auto dungeon = DungeonMap::require(dungeon_uuid);
	auto skill_data  = Data::Skill::require(get_skill_id());
	auto dungon_objec_uuid = owner_object->get_dungeon_object_uuid();
	auto account_uuid      = owner_object->get_owner_uuid();
	auto attack            = owner_object->get_total_attack();
	auto skill_id          = get_skill_id();
	auto utc_now           = Poseidon::get_utc_time();
	auto range_coords      = coords;
	auto warning_time      = skill_data->warning_time;
	auto skill_recycle_damage = boost::make_shared<SkillRecycleDamage>(cast_coord,dungon_objec_uuid,account_uuid,attack,skill_id,utc_now,warning_time*1000,-1,std::move(range_coords));;
	if(skill_recycle_damage){
		dungeon->insert_skill_damage(skill_recycle_damage);
		dungeon->pump_skill_damage();
	}
}

SkillCorrosion::SkillCorrosion(DungeonMonsterSkillId skill_id,const boost::weak_ptr<DungeonObject> owner)
	:Skill(skill_id,owner){
}

SkillCorrosion::~SkillCorrosion(){
}

void  SkillCorrosion::do_effects(){
	PROFILE_ME;

	auto owner_object  = get_owner();
	if(!owner_object){
		LOG_EMPERY_DUNGEON_WARNING("CANNT GET SKILL OWNER, skill_id = ",get_skill_id());
		return;
	}
	auto cast_coord = get_cast_coord();
	if(cast_coord == Coord(0,0)){
		LOG_EMPERY_DUNGEON_WARNING("skill corrosion cast coord is (0,0)");
		return;
	}
	const auto dungeon_uuid = owner_object->get_dungeon_uuid();
	const auto dungeon = DungeonMap::require(dungeon_uuid);
	std::vector<Coord> coords;
	coords.push_back(cast_coord);
	Skill::notify_effects(coords);
	auto cast_params = get_cast_params();
	auto cast_object = dungeon->get_object(DungeonObjectUuid(cast_params));
	if(!cast_object){
		LOG_EMPERY_DUNGEON_WARNING("Skill Corrosion cast object lost, cast_params = ",cast_params," cast_coord = ", cast_coord);
		return;
	}
	auto dungeon_object_uuid = cast_object->get_dungeon_object_uuid();
	auto dungeon_object_owner_account = cast_object->get_owner_uuid();
	const auto buff_data = Data::DungeonBuff::require(ID_BUFF_CORROSION);
	auto utc_now           = Poseidon::get_utc_time();
	auto expired_time      = utc_now + buff_data->continue_time*1000;
	auto dungeon_buff = boost::make_shared<DungeonBuff>(dungeon_uuid,ID_BUFF_CORROSION,dungeon_object_uuid,dungeon_object_owner_account,cast_coord,expired_time);
	LOG_EMPERY_DUNGEON_ERROR("insert skill buff, dungeon_object_uuid = ",dungeon_object_uuid," BUFF ID = ",ID_BUFF_CORROSION);
	dungeon->insert_skill_buff(dungeon_object_uuid,ID_BUFF_CORROSION,std::move(dungeon_buff));
}

SkillReflexInjury::SkillReflexInjury(DungeonMonsterSkillId skill_id,const boost::weak_ptr<DungeonObject> owner)
	:Skill(skill_id,owner){
}

SkillReflexInjury::~SkillReflexInjury(){
}

void  SkillReflexInjury::do_effects(){
	PROFILE_ME;

	auto owner_object  = get_owner();
	if(!owner_object){
		LOG_EMPERY_DUNGEON_WARNING("CANNT GET SKILL OWNER, skill_id = ",get_skill_id());
		return;
	}
	auto cast_coord = owner_object->get_coord();
	if(cast_coord == Coord(0,0)){
		LOG_EMPERY_DUNGEON_WARNING("skill injury cast coord is (0,0)");
		return;
	}
	const auto dungeon_uuid = owner_object->get_dungeon_uuid();
	const auto dungeon = DungeonMap::require(dungeon_uuid);
	std::vector<Coord> coords;
	coords.push_back(cast_coord);
	Skill::notify_effects(coords);
	auto dungeon_object_uuid = owner_object->get_dungeon_object_uuid();
	auto dungeon_object_owner_account = owner_object->get_owner_uuid();
	const auto buff_data = Data::DungeonBuff::require(ID_BUFF_REFLEX_INJURY);
	auto utc_now           = Poseidon::get_utc_time();
	auto expired_time      = utc_now + buff_data->continue_time*1000;
	auto dungeon_buff = boost::make_shared<DungeonBuff>(dungeon_uuid,ID_BUFF_REFLEX_INJURY,dungeon_object_uuid,dungeon_object_owner_account,cast_coord,expired_time);
	dungeon->insert_skill_buff(dungeon_object_uuid,ID_BUFF_REFLEX_INJURY,std::move(dungeon_buff));
}

SkillRage::SkillRage(DungeonMonsterSkillId skill_id,const boost::weak_ptr<DungeonObject> owner)
	:Skill(skill_id,owner){
}

SkillRage::~SkillRage(){
}

void  SkillRage::do_effects(){
	PROFILE_ME;

	auto owner_object  = get_owner();
	if(!owner_object){
		LOG_EMPERY_DUNGEON_WARNING("CANNT GET SKILL OWNER, skill_id = ",get_skill_id());
		return;
	}
	auto cast_coord = owner_object->get_coord();
	if(cast_coord == Coord(0,0)){
		LOG_EMPERY_DUNGEON_WARNING("skill rage cast coord is (0,0)");
		return;
	}
	const auto dungeon_uuid = owner_object->get_dungeon_uuid();
	const auto dungeon = DungeonMap::require(dungeon_uuid);
	std::vector<Coord> coords;
	coords.push_back(cast_coord);
	Skill::notify_effects(coords);
	auto dungeon_object_uuid = owner_object->get_dungeon_object_uuid();
	auto dungeon_object_owner_account = owner_object->get_owner_uuid();
	const auto buff_data = Data::DungeonBuff::require(ID_BUFF_RAGE);
	auto utc_now           = Poseidon::get_utc_time();
	auto expired_time      = utc_now + buff_data->continue_time*1000;
	auto dungeon_buff = boost::make_shared<DungeonBuff>(dungeon_uuid,ID_BUFF_RAGE,dungeon_object_uuid,dungeon_object_owner_account,cast_coord,expired_time);
	dungeon->insert_skill_buff(dungeon_object_uuid,ID_BUFF_RAGE,std::move(dungeon_buff));
}

SkillSummonSkulls::SkillSummonSkulls(DungeonMonsterSkillId skill_id,const boost::weak_ptr<DungeonObject> owner)
	:Skill(skill_id,owner){
}

SkillSummonSkulls::~SkillSummonSkulls(){
}

void  SkillSummonSkulls::do_effects(){
	PROFILE_ME;

	auto owner_object  = get_owner();
	if(!owner_object){
		LOG_EMPERY_DUNGEON_WARNING("CANNT GET SKILL OWNER, skill_id = ",get_skill_id());
		return;
	}
	const auto dungeon_uuid = owner_object->get_dungeon_uuid();
	const auto dungeon = DungeonMap::require(dungeon_uuid);
	const auto dungeon_client = dungeon->get_dungeon_client();
	if(!dungeon_client){
		LOG_EMPERY_DUNGEON_WARNING("skill do damge, dungeon client is null");
		return;
	}
	auto origin_coord = owner_object->get_coord();
	auto skill_data  = Data::Skill::require(get_skill_id());
	auto range = skill_data->cast_range;
	std::vector<Coord> coords;
	for(unsigned i = 1; i <= range; i++){
		get_surrounding_coords(coords,origin_coord,i);
	}
	std::vector<Coord> valid_birth_coords;
	for(auto it = coords.begin(); it != coords.end(); ++it){
		auto coord = *it;
		bool valid_coord = dungeon->check_valid_coord_for_birth(coord);
		if(valid_coord){
			valid_birth_coords.push_back(std::move(coord));
		}
	}
	if(valid_birth_coords.size() < 1){
		LOG_EMPERY_DUNGEON_WARNING("server can not get enough valid coords for skulls birth");
		return;
	}
	
	std::vector<Coord> cast_coords;
	std::random_shuffle(valid_birth_coords.begin(),valid_birth_coords.end());
	auto random_lattice = skill_data->random_lattice;
	for(unsigned i = 0; i < valid_birth_coords.size(); ++ i){
		if(i < random_lattice){
			cast_coords.push_back(coords.at(i));
		}
	}
	Skill::notify_effects(cast_coords);
	for(unsigned i = 0; i < cast_coords.size(); ++i){
		try{
			auto &coord = cast_coords.at(i);
			Msg::DS_DungeonCreateMonster msgCreateMonster;
			msgCreateMonster.dungeon_uuid       = dungeon_uuid.str();
			msgCreateMonster.map_object_type_id = skill_data->pet_id.get();
			msgCreateMonster.x                  = coord.x();
			msgCreateMonster.y                  = coord.y();
			msgCreateMonster.dest_x             = coord.x();
			msgCreateMonster.dest_y             = coord.y();
			msgCreateMonster.tag                = "0";
			dungeon_client->send(msgCreateMonster);
		} catch(std::exception &e){
			LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
			dungeon_client->shutdown(e.what());
		}
		
	}
}


SkillSummonMonster::SkillSummonMonster(DungeonMonsterSkillId skill_id,const boost::weak_ptr<DungeonObject> owner)
	:Skill(skill_id,owner){
}

SkillSummonMonster::~SkillSummonMonster(){
}

void  SkillSummonMonster::do_effects(){
	PROFILE_ME;

	auto owner_object  = get_owner();
	if(!owner_object){
		LOG_EMPERY_DUNGEON_WARNING("CANNT GET SKILL OWNER, skill_id = ",get_skill_id());
		return;
	}
	const auto dungeon_uuid = owner_object->get_dungeon_uuid();
	const auto dungeon = DungeonMap::require(dungeon_uuid);
	const auto dungeon_client = dungeon->get_dungeon_client();
	if(!dungeon_client){
		LOG_EMPERY_DUNGEON_WARNING("skill do damge, dungeon client is null");
		return;
	}
	auto origin_coord = owner_object->get_coord();
	auto skill_data  = Data::Skill::require(get_skill_id());
	auto range = skill_data->cast_range;
	std::vector<Coord> coords;
	for(unsigned i = 1; i <= range; i++){
		get_surrounding_coords(coords,origin_coord,i);
	}
	std::random_shuffle(coords.begin(),coords.end());
	std::vector<Coord> valid_birth_coords;
	for(auto it = coords.begin(); it != coords.end(); ++it){
		auto coord = *it;
		bool valid_coord = dungeon->check_valid_coord_for_birth(coord);
		if(valid_coord){
			valid_birth_coords.push_back(std::move(coord));
		}
	}
	if(valid_birth_coords.size() < 1){
		LOG_EMPERY_DUNGEON_WARNING("server can not get enough valid coords for skulls birth");
		return;
	}
	
	std::vector<Coord> cast_coords;
	std::random_shuffle(valid_birth_coords.begin(),valid_birth_coords.end());
	auto random_lattice = skill_data->random_lattice;
	for(unsigned i = 0; i < valid_birth_coords.size(); ++ i){
		if(i < random_lattice){
			cast_coords.push_back(coords.at(i));
		}
	}
	Skill::notify_effects(cast_coords);
	for(unsigned i = 0; i < cast_coords.size(); ++i){
		try{
			auto &coord = cast_coords.at(i);
			Msg::DS_DungeonCreateMonster msgCreateMonster;
			msgCreateMonster.dungeon_uuid       = dungeon_uuid.str();
			msgCreateMonster.map_object_type_id = skill_data->pet_id.get();
			msgCreateMonster.x                  = coord.x();
			msgCreateMonster.y                  = coord.y();
			msgCreateMonster.dest_x             = coord.x();
			msgCreateMonster.dest_y             = coord.y();
			msgCreateMonster.tag                = "0";
			dungeon_client->send(msgCreateMonster);
		} catch(std::exception &e){
			LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
			dungeon_client->shutdown(e.what());
		}
		
	}
}


SkillSoulAttack::SkillSoulAttack(DungeonMonsterSkillId skill_id,const boost::weak_ptr<DungeonObject> owner)
	:Skill(skill_id,owner){
}

SkillSoulAttack::~SkillSoulAttack(){
}

void  SkillSoulAttack::do_effects(){
	PROFILE_ME;

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
	std::vector<Coord> cast_coords;
	auto random_lattice = skill_data->random_lattice;
	std::random_shuffle(coords.begin(),coords.end());
	for(unsigned i = 0; i < coords.size(); ++ i){
		if(i < random_lattice){
			cast_coords.push_back(coords.at(i));
		}
	}
	Skill::notify_effects(cast_coords);
	const auto dungeon_uuid = owner_object->get_dungeon_uuid();
	const auto dungeon = DungeonMap::require(dungeon_uuid);
	auto dungon_objec_uuid = owner_object->get_dungeon_object_uuid();
	auto account_uuid      = owner_object->get_owner_uuid();
	auto attack            = owner_object->get_total_attack();
	auto skill_id          = get_skill_id();
	auto utc_now           = Poseidon::get_utc_time();
	auto range_coords      = cast_coords;
	auto warning_time      = skill_data->warning_time;
	auto skill_recycle_damage = boost::make_shared<SkillRecycleDamage>(origin_coord,dungon_objec_uuid,account_uuid,attack,skill_id,utc_now + warning_time*1000,warning_time*1000,1,std::move(range_coords));;
	if(skill_recycle_damage){
		dungeon->insert_skill_damage(skill_recycle_damage);
		dungeon->pump_skill_damage();
	}
}

SkillEnergySphere::SkillEnergySphere(DungeonMonsterSkillId skill_id,const boost::weak_ptr<DungeonObject> owner)
	:Skill(skill_id,owner){
}

SkillEnergySphere::~SkillEnergySphere(){
}

void  SkillEnergySphere::do_effects(){
	PROFILE_ME;

	auto owner_object  = get_owner();
	if(!owner_object){
		LOG_EMPERY_DUNGEON_WARNING("CANNT GET SKILL OWNER, skill_id = ",get_skill_id());
		return;
	}
	auto origin_coord = owner_object->get_coord();
	auto skill_data  = Data::Skill::require(get_skill_id());
	auto range = skill_data->skill_range;
	std::vector<Coord> coords;
	for(unsigned i = 0; i <= range; i++){
		get_surrounding_coords(coords,origin_coord,i);
	}
	if(coords.empty()){
		LOG_EMPERY_DUNGEON_WARNING("energy sphere range is empty ? skill_range = ",range);
		return;
	}
	Skill::notify_effects(coords);
	const auto dungeon_uuid = owner_object->get_dungeon_uuid();
	const auto dungeon = DungeonMap::require(dungeon_uuid);
	auto dungon_objec_uuid = owner_object->get_dungeon_object_uuid();
	auto account_uuid      = owner_object->get_owner_uuid();
	auto attack            = owner_object->get_total_attack();
	auto skill_id          = get_skill_id();
	auto utc_now           = Poseidon::get_utc_time();
	auto range_coords      = coords;
	auto warning_time      = skill_data->warning_time;
	auto skill_recycle_damage = boost::make_shared<SkillRecycleDamage>(origin_coord,dungon_objec_uuid,account_uuid,attack,skill_id,utc_now + warning_time*1000,warning_time*1000,1,std::move(range_coords));;
	if(skill_recycle_damage){
		LOG_EMPERY_DUNGEON_FATAL("ADD SKILL ENERGY SPHERE DAMAGE,warning_time = ",warning_time);
		dungeon->insert_skill_damage(skill_recycle_damage);
		dungeon->pump_skill_damage();
	}
}











}
