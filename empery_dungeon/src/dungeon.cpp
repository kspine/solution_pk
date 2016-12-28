#include "precompiled.hpp"
#include "dungeon.hpp"
#include "singletons/dungeon_map.hpp"
#include "src/dungeon_object.hpp"
#include "src/dungeon_client.hpp"
#include "src/data/dungeon.hpp"
#include "src/data/trigger.hpp"
#include "../../empery_center/src/msg/ds_dungeon.hpp"
#include "trigger.hpp"
#include <poseidon/json.hpp>
#include "mmain.hpp"
#include <poseidon/cbpp/status_codes.hpp>
#include "dungeon_utilities.hpp"
#include "src/data/global.hpp"
#include "src/data/dungeon_object.hpp"
#include "src/data/dungeon_buff.hpp"
#include "src/dungeon_buff.hpp"
#include "../../empery_center/src/attribute_ids.hpp"
#include "skill.hpp"
#include "src/data/skill.hpp"
#include "defense_matrix.hpp"

namespace EmperyDungeon {
namespace Msg = ::EmperyCenter::Msg;

namespace {
	void notify_dungeon_object_updated(const boost::shared_ptr<DungeonObject> &dungeon_object, const boost::shared_ptr<DungeonClient> &dungeon_client){
		PROFILE_ME;

		Msg::DS_DungeonUpdateObjectAction msg;
		msg.dungeon_uuid        = dungeon_object->get_dungeon_uuid().str();
		msg.dungeon_object_uuid = dungeon_object->get_dungeon_object_uuid().str();
		msg.x                   = dungeon_object->get_coord().x();
		msg.y                   = dungeon_object->get_coord().y();
		msg.action              = dungeon_object->get_action();
		msg.param               = dungeon_object->get_action_param();
		dungeon_client->send(msg);
	}
}


Dungeon::Dungeon(DungeonUuid dungeon_uuid, DungeonTypeId dungeon_type_id,const boost::shared_ptr<DungeonClient> &dungeon,AccountUuid founder_uuid,std::uint64_t create_time,std::uint64_t finish_count,std::uint64_t expiry_time)
	: m_dungeon_uuid(dungeon_uuid), m_dungeon_type_id(dungeon_type_id)
	, m_dungeon_client(dungeon)
	, m_founder_uuid(founder_uuid)
	, m_create_dungeon_time(create_time)
	, m_finish_count(finish_count)
	, m_expiry_time(expiry_time)
	, m_dungeon_state(S_INIT)
	, m_monster_removed_count(0)
	, m_fight_state(FIGHT_START)
{
}

Dungeon::~Dungeon(){
}

void Dungeon::pump_status(){
	PROFILE_ME;

	pump_triggers();
	pump_triggers_damage();
	pump_skill_damage();
	pump_defense_matrix();
}

void Dungeon::pump_triggers(){
	std::vector<boost::shared_ptr<Trigger>> triggers_activated;
	triggers_activated.reserve(m_triggers.size());
	const auto utc_now = Poseidon::get_utc_time();
	for(auto it = m_triggers.begin(); it != m_triggers.end(); ++it){
		const auto &trigger = it->second;

		if(!trigger->activated){
			continue;
		}
		std::uint64_t interval = utc_now - trigger->activated_time;
		if(interval < trigger->delay){
			continue;
		}
		trigger->activated = false;
		trigger->activated_time = 0;
		triggers_activated.emplace_back(trigger);
	}
	for(auto it = triggers_activated.begin(); it != triggers_activated.end(); ++it){
		const auto &trigger = *it;
		notify_triggers_executive(trigger);
		try {
			// 根据行为选择操作
			std::deque<TriggerAction> &actions = trigger->actions;
			for(auto it = actions.begin(); it != actions.end();++it){
				auto &trigger_action = *it;
				trigger_action.dungeon_params = trigger->params;
				on_triggers_action(trigger_action);
			}
		} catch(std::exception &e){
			// log
		}
	}
}

void Dungeon::pump_triggers_damage(){
	std::vector<boost::shared_ptr<TriggerDamage>> triggers_damaged;
	triggers_damaged.reserve(m_triggers_damages.size());
	const auto utc_now = Poseidon::get_utc_time();
	for(auto it = m_triggers_damages.begin(); it != m_triggers_damages.end();){
		const auto &damage = *it;

		if(damage->loop == 0){
			it = m_triggers_damages.erase(it);
		}else{
			if((utc_now < damage->next_damage_time)){
				++it;
				continue;
			}
			damage->loop--;
			damage->next_damage_time = utc_now + damage->delay;
			triggers_damaged.emplace_back(damage);
			++it;
		}
	}
	if(m_fight_state == FIGHT_PAUSE){
		return;
	}
	for(auto it = triggers_damaged.begin(); it != triggers_damaged.end(); ++it){
		const auto &damage = *it;
		try {
			// 根据行为选择操作
			do_triggers_damage(damage);
		} catch(std::exception &e){
			// log
		}
	}
}

void Dungeon::set_founder_uuid(AccountUuid founder_uuid){
	PROFILE_ME;

	m_founder_uuid = founder_uuid;

	DungeonMap::update(virtual_shared_from_this<Dungeon>(), false);
}
void Dungeon::set_expiry_time(std::uint64_t expiry_time){
	PROFILE_ME;

	m_expiry_time = expiry_time;
}

void Dungeon::update_damage_solider(DungeonObjectTypeId dungeon_object_type_id,std::uint64_t damage_solider){
	PROFILE_ME;

	auto it = m_damage_solider.find(dungeon_object_type_id);
	if(it == m_damage_solider.end()){
		m_damage_solider.emplace(dungeon_object_type_id, damage_solider);
		return;
	}else{
		m_damage_solider.at(it->first) = it->second + damage_solider;
	}
}

std::uint64_t Dungeon::get_total_damage_solider(){
	PROFILE_ME;

	std::uint64_t total_damage_solider = 0;
	for(auto it = m_damage_solider.begin(); it != m_damage_solider.end(); ++it){
		total_damage_solider += it->second;
	}
	return total_damage_solider;
}

void Dungeon::set_fight_state(FightState fight_state){
	PROFILE_ME;

	m_fight_state = fight_state;
}

void Dungeon::check_move_pass(Coord coord,std::string params,bool is_monster){
	check_triggers_move_pass(coord,params,is_monster);
	check_skill_damage_move_pass(coord,params,is_monster);
}

boost::shared_ptr<DungeonObject> Dungeon::get_object(DungeonObjectUuid dungeon_object_uuid) const {
	PROFILE_ME;

	const auto it = m_objects.find(dungeon_object_uuid);
	if(it == m_objects.end()){
		LOG_EMPERY_DUNGEON_DEBUG("Dungeon object not found: dungeon_object_uuid = ", dungeon_object_uuid);
		return { };
	}
	return it->second;
}

boost::shared_ptr<DungeonObject> Dungeon::get_object(Coord coord) const{
	PROFILE_ME;

	for(auto it = m_objects.begin(); it != m_objects.end(); ++it){
		if(it->second->get_coord() == coord){
			return it->second;
		}
	}
	return {};
}
void Dungeon::get_objects_all(std::vector<boost::shared_ptr<DungeonObject>> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_objects.size());
	for(auto it = m_objects.begin(); it != m_objects.end(); ++it){
		ret.emplace_back(it->second);
	}
}

void Dungeon::get_dungeon_objects_by_rectangle(std::vector<boost::shared_ptr<DungeonObject>> &ret,Rectangle rectangle) const{
	PROFILE_ME;

	for(auto it = m_objects.begin(); it != m_objects.end(); ++it){
		auto &dungeon_object = it->second;
		auto object_coord = dungeon_object->get_coord();
		if(rectangle.hit_test(object_coord)){
			ret.emplace_back(it->second);
		}
	}
}

void Dungeon::get_dungeon_objects_by_account(std::vector<boost::shared_ptr<DungeonObject>> &ret,AccountUuid account_uuid){
	PROFILE_ME;

	for(auto it = m_objects.begin(); it != m_objects.end(); ++it){
		auto &dungeon_object = it->second;
		if(dungeon_object->get_owner_uuid() == account_uuid){
			ret.emplace_back(it->second);
		}
	}
}

void Dungeon::insert_object(const boost::shared_ptr<DungeonObject> &dungeon_object){
	PROFILE_ME;

	const auto dungeon_uuid = get_dungeon_uuid();
	if(dungeon_object->get_dungeon_uuid() != dungeon_uuid){
		LOG_EMPERY_DUNGEON_WARNING("This dungeon object does not belong to this dungeon!");
		DEBUG_THROW(Exception, sslit("This dungeon object does not belong to this dungeon"));
	}

	const auto dungeon_object_uuid = dungeon_object->get_dungeon_object_uuid();

	LOG_EMPERY_DUNGEON_DEBUG("Inserting dungeon object: dungeon_object_uuid = ", dungeon_object_uuid, ", dungeon_uuid = ", dungeon_uuid);
	const auto result = m_objects.emplace(dungeon_object_uuid, dungeon_object);
	if(!result.second){
		LOG_EMPERY_DUNGEON_WARNING("Dungeon object already exists: dungeon_object_uuid = ", dungeon_object_uuid, ", dungeon_uuid = ", dungeon_uuid);
		DEBUG_THROW(Exception, sslit("Dungeon object already exists"));
	}
}

void Dungeon::update_object(const boost::shared_ptr<DungeonObject> &dungeon_object, bool throws_if_not_exists){
	PROFILE_ME;

	const auto dungeon_uuid = get_dungeon_uuid();
	if(dungeon_object->get_dungeon_uuid() != dungeon_uuid){
		LOG_EMPERY_DUNGEON_WARNING("This dungeon object does not belong to this dungeon!");
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("This dungeon object does not belong to this dungeon"));
		}
		return;
	}

	const auto dungeon_object_uuid = dungeon_object->get_dungeon_object_uuid();

	const auto it = m_objects.find(dungeon_object_uuid);
	if(it == m_objects.end()){
		LOG_EMPERY_DUNGEON_TRACE("Dungeon object not found: dungeon_object_uuid = ", dungeon_object_uuid, ", dungeon_uuid = ", dungeon_uuid);
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Dungeon object not found"));
		}
		return;
	}
	m_objects.at(it->first) = dungeon_object;
	auto dungeon_client = get_dungeon_client();
	if(dungeon_client){
		notify_dungeon_object_updated(dungeon_object,dungeon_client);
	}
}

void Dungeon::replace_dungeon_object_no_synchronize(const boost::shared_ptr<DungeonObject> &dungeon_object){
	PROFILE_ME;

	const auto dungeon_uuid = get_dungeon_uuid();
	if(dungeon_object->get_dungeon_uuid() != dungeon_uuid){
		LOG_EMPERY_DUNGEON_WARNING("This dungeon object does not belong to this dungeon!");
		return;
	}
	const auto dungeon_object_uuid = dungeon_object->get_dungeon_object_uuid();
	const auto it = m_objects.find(dungeon_object_uuid);
	if(it == m_objects.end()){
		m_objects.emplace(dungeon_object_uuid, dungeon_object);
		return;
	}else{
		m_objects.at(it->first) = dungeon_object;
	}
}

void Dungeon::remove_dungeon_object_no_synchronize(DungeonObjectUuid dungeon_object_uuid){
	PROFILE_ME;

	const auto it = m_objects.find(dungeon_object_uuid);
	if(it == m_objects.end()){
		LOG_EMPERY_DUNGEON_TRACE("Dungeon object not found: dungeon_object_uuid = ", dungeon_object_uuid, ", dungeon_uuid = ", get_dungeon_uuid());
		return;
	}else{
		auto &dungeon_object = it->second;
		if(dungeon_object->is_monster()){
			m_monster_removed_count += 1;
		}
		LOG_EMPERY_DUNGEON_ERROR("check die skill");
		dungeon_object->do_die_skill();
		m_die_objects.push_back(dungeon_object->get_tag());
		m_objects.erase(it);
	}
}

bool Dungeon::check_all_die(bool is_monster){
	PROFILE_ME;
	std::uint64_t count = 0;
	for(auto it = m_objects.begin(); it != m_objects.end(); ++it){
		auto &dungeon_object = it->second;
		if(dungeon_object->is_monster() == is_monster){
			count +=1;
		}
	}
	if(count ==0){
		return true;
	}
	return false;
}

bool Dungeon::check_valid_coord_for_birth(const Coord &src_coord){
	bool pos_occuried = false;
	bool pos_passable = false;
	bool pos_blocks = false;
	//判断src_coord上是否有对象
	for(auto it = m_objects.begin(); it != m_objects.end(); ++it){
		auto &dungeon_object = it->second;
		if(dungeon_object->get_coord() == src_coord){
			pos_occuried = true;
			break;
		}
	}
	//判断相应点是否可通行
	pos_passable = get_dungeon_coord_passable(get_dungeon_type_id(),src_coord);

	//判断是否是触发器器设置的阻塞点
	pos_blocks = is_dungeon_blocks_coord(src_coord);
	if(!pos_occuried && pos_passable && !pos_blocks){
		return true;
	}
	return false;
}

bool Dungeon::get_monster_birth_coord(const Coord &src_coord,Coord &dest_coord){
	PROFILE_ME;
	bool src_valid =  check_valid_coord_for_birth(src_coord);
	if(src_valid){
		dest_coord = src_coord;
		return true;
	}
	std::vector<Coord> surrounding;
	for(unsigned i = 1; i <= 3; i++){
		get_surrounding_coords(surrounding,src_coord, i);
	}
	for(auto it = surrounding.begin(); it != surrounding.end();++it){
		src_valid =  check_valid_coord_for_birth(*it);
		if(src_valid){
			dest_coord = *it;
			return true;
		}
	}
	return false;
}

boost::shared_ptr<DungeonBuff> Dungeon::get_dungeon_buff(const Coord coord) const{
	PROFILE_ME;

	const auto it = m_dungeon_buffs.find(coord);
	if(it == m_dungeon_buffs.end()){
		LOG_EMPERY_DUNGEON_DEBUG("Dungeon buff not found: coord = ", coord);
		return { };
	}
	return it->second;
}

void Dungeon::insert_dungeon_buff(const boost::shared_ptr<DungeonBuff> &dungeon_buff){
	PROFILE_ME;

	const auto dungeon_uuid = get_dungeon_uuid();
	if(dungeon_buff->get_dungeon_uuid() != dungeon_uuid){
		LOG_EMPERY_DUNGEON_WARNING("This dungeon buff does not belong to this dungeon!");
		DEBUG_THROW(Exception, sslit("This dungeon buff does not belong to this dungeon"));
	}

	const auto coord = dungeon_buff->get_coord();

	LOG_EMPERY_DUNGEON_DEBUG("Inserting dungeon buff: coord = ", coord, ", dungeon_uuid = ", dungeon_uuid);
	const auto result = m_dungeon_buffs.emplace(coord, dungeon_buff);
	if(!result.second){
		LOG_EMPERY_DUNGEON_WARNING("Dungeon buff already exists: coord = ", coord, ", dungeon_uuid = ", dungeon_uuid);
		DEBUG_THROW(Exception, sslit("Dungeon buff already exists"));
	}
}

void Dungeon::update_dungeon_buff(const boost::shared_ptr<DungeonBuff> &dungeon_buff, bool throws_if_not_exists){
	PROFILE_ME;

	const auto dungeon_uuid = get_dungeon_uuid();
	if(dungeon_buff->get_dungeon_uuid() != dungeon_uuid){
		LOG_EMPERY_DUNGEON_WARNING("This dungeon buff does not belong to this dungeon!");
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("This dungeon buff does not belong to this dungeon"));
		}
		return;
	}

	const auto coord = dungeon_buff->get_coord();

	const auto it = m_dungeon_buffs.find(coord);
	if(it == m_dungeon_buffs.end()){
		LOG_EMPERY_DUNGEON_TRACE("Dungeon buff not found: coord = ", coord, ", dungeon_uuid = ", dungeon_uuid);
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Dungeon buff not found"));
		}
		return;
	}
	m_dungeon_buffs.at(it->first) = dungeon_buff;
}

void Dungeon::replace_dungeon_buff_no_synchronize(const boost::shared_ptr<DungeonBuff> &dungeon_buff){
	PROFILE_ME;

	const auto dungeon_uuid = get_dungeon_uuid();
	if(dungeon_buff->get_dungeon_uuid() != dungeon_uuid){
		LOG_EMPERY_DUNGEON_WARNING("This dungeon buff does not belong to this dungeon!");
		return;
	}
	const auto coord = dungeon_buff->get_coord();
	const auto it = m_dungeon_buffs.find(coord);
	if(it == m_dungeon_buffs.end()){
		m_dungeon_buffs.emplace(coord, dungeon_buff);
		return;
	}else{
		m_dungeon_buffs.at(it->first) = dungeon_buff;
	}
}

void Dungeon::remove_dungeon_buff_no_synchronize(const Coord coord){
	PROFILE_ME;

	const auto it = m_dungeon_buffs.find(coord);
	if(it == m_dungeon_buffs.end()){
		LOG_EMPERY_DUNGEON_TRACE("Dungeon buff not found: coord = ", coord, ", dungeon_uuid = ", get_dungeon_uuid());
		return;
	}else{
		m_dungeon_buffs.erase(it);
	}
}

std::uint64_t Dungeon::get_dungeon_attribute(DungeonObjectUuid create_uuid,AccountUuid owner_uuid,AttributeId attribute_id){
	PROFILE_ME;
	const auto utc_now = Poseidon::get_utc_time();
	std::uint64_t total_attribute = 0;
	for(auto it = m_dungeon_buffs.begin();it != m_dungeon_buffs.end(); ++it){
		auto &dungeon_buff = it->second;
		if(dungeon_buff->get_expired_time() < utc_now){
			continue;
		}
		try{
			auto buff_type_id = dungeon_buff->get_dungeon_buff_type_id();
			const auto data_buff = Data::DungeonBuff::require(buff_type_id);
			if(((data_buff->target == 1) && (create_uuid == dungeon_buff->get_create_uuid())) ||
				((data_buff->target == 2) && (owner_uuid == dungeon_buff->get_create_owner_uuid()))){
				if(data_buff->attribute_id == attribute_id){
					total_attribute += data_buff->value;
				}
			}
		}catch(std::exception &e){
			LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
		}
	}
	for(auto it = m_skill_buffs.begin(); it != m_skill_buffs.end(); ++it){
		auto &dungeon_buff = it->second;
		if(dungeon_buff->get_expired_time() < utc_now){
			continue;
		}
		try{
			auto buff_type_id = dungeon_buff->get_dungeon_buff_type_id();
			const auto data_buff = Data::DungeonBuff::require(buff_type_id);
			if(create_uuid == dungeon_buff->get_create_uuid()){
				if(data_buff->attribute_id == attribute_id){
					total_attribute += data_buff->value;
				}
			}
		}catch(std::exception &e){
			LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
		}
	}
	return total_attribute;
}

bool Dungeon::is_dungeon_blocks_coord(const Coord coord){
	auto it = m_dungeon_blocks.find(coord);
	if(it == m_dungeon_blocks.end()){
		return false;
	}
	return true;
}

void Dungeon::init_triggers(){
	PROFILE_ME;

	const auto dungeon_data = Data::Dungeon::require(get_dungeon_type_id());
	std::vector<boost::shared_ptr<const Data::Trigger>> ret;
	Data::Trigger::get_all(ret,dungeon_data->dungeon_trigger);
	if(ret.empty()){
		LOG_EMPERY_DUNGEON_WARNING("dungeon have empty trigger,dungeon_type_id = ",get_dungeon_type_id());
		return;
	}
	const auto utc_now = Poseidon::get_utc_time();
	for(auto it = ret.begin(); it != ret.end(); ++it){
		auto trigger_id   = (*it)->trigger_id;
		std::uint64_t delay = (*it)->delay;
		auto status   = (*it)->status;
		//第一次进只读status == 0,1
		if(m_finish_count == 0 && status == 2){
			LOG_EMPERY_DUNGEON_WARNING("trigger was pass,because status = 2 and finish_count = ",m_finish_count);
			continue;
		//第二次进只读status == 0,2
		}else if( m_finish_count > 0 && status == 1){
			LOG_EMPERY_DUNGEON_WARNING("trigger was pass,because status = 1 and finish_count = ",m_finish_count);
			continue;
		}
		TriggerCondition trigger_conditon;
		trigger_conditon.type      = static_cast<TriggerCondition::Type>((*it)->type);
		trigger_conditon.params  = (*it)->condition;
		auto effect = (*it)->effect;
		auto effect_param = (*it)->effect_params;
		std::deque<TriggerAction> actions;
		auto activated = (*it)->activated;
		auto times  = (*it)->times;
		auto open   = (*it)->open;
		parse_triggers_action(actions,std::move(effect),std::move(effect_param));
		std::uint64_t activated_time = 0;
		if(activated){
			activated_time = utc_now;
		}
		auto triggers = boost::make_shared<Trigger>(get_dungeon_uuid(),trigger_id,delay,std::move(trigger_conditon),std::move(actions),activated,activated_time,times,open);
		const auto result = m_triggers.emplace(trigger_id, std::move(triggers));
		if(!result.second){
			LOG_EMPERY_DUNGEON_WARNING("Dungeon trigger already exists: dungeon_type_id = ", get_dungeon_type_id(), ", trigger_id = ", trigger_id);
			DEBUG_THROW(Exception, sslit("Dungeon trigger already exists"));
		}
	}
}

void Dungeon::forcast_triggers(const boost::shared_ptr<Trigger> &trigger,std::uint64_t now){
	if(!trigger){
		return;
	}
	const auto dungeon_client = get_dungeon_client();
	if(dungeon_client){
		try{
			const auto executive_time = now + trigger->delay;
			Msg::DS_DungeonTriggerEffectForcast msg;
			msg.dungeon_uuid    = get_dungeon_uuid().str();
			msg.trigger_id      = trigger->trigger_id;
			msg.executive_time  = executive_time;
			for(auto it = trigger->actions.begin(); it != trigger->actions.end(); ++it){
				auto &effects = *msg.effects.emplace(msg.effects.end());
				effects.effect_type = it->type;
			}
			dungeon_client->send(msg);
		}
		catch(std::exception &e){
			LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
			dungeon_client->shutdown(e.what());
		}
	}
}

void Dungeon::parse_triggers_action(std::deque<TriggerAction> &actions,std::string effect,std::string effect_param){
	PROFILE_ME;

	try{
		std::istringstream iss_effect(effect);
		auto effect_array = Poseidon::JsonParser::parse_array(iss_effect);
		for(unsigned i  = 0; i < effect_array.size(); ++i){
			TriggerAction triggerAction;
			triggerAction.type           = static_cast<TriggerAction::Type>(effect_array.at(i).get<double>());
			if(!effect_param.empty()){
				std::istringstream iss_effect_param(effect_param);
				auto effect_param_array = Poseidon::JsonParser::parse_array(iss_effect_param);
				if(i <= (effect_param_array.size() -1) ){
					triggerAction.params = effect_param_array.at(i).dump();
				}
			}
			LOG_EMPERY_DUNGEON_DEBUG("parse triggers_action ,raw_effect = ",effect," raw_effect_param = ",effect_param," type = ", triggerAction.type, " params = ", triggerAction.params);
			actions.push_back(std::move(triggerAction));
		}
	} catch(std::exception &e){
				LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
	}
}

void Dungeon::check_triggers_enter_dungeon(){
	PROFILE_ME;

	const auto utc_now = Poseidon::get_utc_time();
	for(auto it = m_triggers.begin(); it != m_triggers.end(); ++it){
		const auto &trigger = it->second;
		if((trigger->open) && (!trigger->activated) && (trigger->condition.type == TriggerCondition::C_ENTER_DUNGEON)&& (trigger->times != 0)){
			trigger->activated = true;
			trigger->activated_time = utc_now;
			trigger->times -= 1;
			forcast_triggers(trigger,utc_now);
		}
	}
	pump_triggers();
}

void Dungeon::check_triggers_move_pass(Coord coord,std::string params,bool isMonster){
	PROFILE_ME;

	try{
		const auto utc_now = Poseidon::get_utc_time();
		for(auto it = m_triggers.begin(); it != m_triggers.end(); ++it){
			const auto &trigger = it->second;
			if((trigger->open) && (!trigger->activated) && (trigger->condition.type == TriggerCondition::C_DUNGEON_OBJECT_PASS)&&(trigger->times != 0)){
				try{
					std::istringstream iss_params(trigger->condition.params);
					auto params_array = Poseidon::JsonParser::parse_array(iss_params);
					int index = params_array.at(0).get<double>();
					if((index == 2) && !isMonster){
						return;
					}
					bool pass = false;
					for(unsigned i = 1; i < params_array.size();++i ){
						auto &elem = params_array.at(i).get<Poseidon::JsonArray>();
						auto x = static_cast<int>(elem.at(0).get<double>());
						auto y = static_cast<int>(elem.at(1).get<double>());
						if((coord.x() == x) && (coord.y() == y)){
							pass = true;
							break;
						}
					}
					if(!pass){
						continue;
					}
				}catch(std::exception &e){
					LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
				}
				trigger->activated = true;
				trigger->activated_time = utc_now;
				trigger->times -= 1;
				trigger->params = params;
				forcast_triggers(trigger,utc_now);
			}
		}
		pump_triggers();
	} catch(std::exception &e){
				LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
	}
}

void Dungeon::check_triggers_hp(std::string tag,std::uint64_t total_hp,std::uint64_t old_hp, std::uint64_t new_hp){
	PROFILE_ME;

	try{
		const auto utc_now = Poseidon::get_utc_time();
		if(old_hp <= new_hp){
			return;
		}
		for(auto it = m_triggers.begin(); it != m_triggers.end(); ++it){
			const auto &trigger = it->second;
			if((trigger->open) && !trigger->activated && (trigger->condition.type == TriggerCondition::C_DUNGEON_OBJECT_HP) &&(trigger->times != 0)){
				try{
					LOG_EMPERY_DUNGEON_DEBUG("check_triggers_hp,conditon params = ", trigger->condition.params);
					std::istringstream iss_params(trigger->condition.params);
					auto params_array = Poseidon::JsonParser::parse_array(iss_params);
					auto dest_tag = boost::lexical_cast<std::string>(params_array.at(0).get<double>());
					auto dest_hp  = static_cast<std::uint64_t>(total_hp*params_array.at(1).get<double>());
					if((dest_tag != tag) || (dest_hp >= old_hp) || (dest_hp < new_hp)){
						continue;
					}
				}catch(std::exception &e){
					LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
				}
				trigger->activated = true;
				trigger->activated_time = utc_now;
				trigger->times -= 1;
				forcast_triggers(trigger,utc_now);
			}
		}
		pump_triggers();
	} catch(std::exception &e){
			LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
	}
}

void Dungeon::check_triggers_dungeon_finish(){
	PROFILE_ME;

	try{
		const auto utc_now = Poseidon::get_utc_time();
		for(auto it = m_triggers.begin(); it != m_triggers.end(); ++it){
			const auto &trigger = it->second;
			if(trigger->open && !trigger->activated && (trigger->condition.type == TriggerCondition::C_DUNGEON_FINISH)&&(trigger->times != 0)){
				try{
					std::istringstream iss_params(trigger->condition.params);
					auto params_array = Poseidon::JsonParser::parse_array(iss_params);
					auto type = boost::lexical_cast<std::uint64_t>(params_array.at(0).get<double>());
					if(type == 0){
						//伤兵
						auto threshold = boost::lexical_cast<std::uint64_t>(params_array.at(1).get<double>());
						if(get_total_damage_solider() > threshold){
							continue;
						}
					}else if(type == 1){
						//副本限时完成
						auto threshold = boost::lexical_cast<std::uint64_t>(params_array.at(1).get<double>());
						auto interval = utc_now - get_create_dungeon_time();
						if(interval > threshold){
							continue;
						}
					}else if(type == 2){
						//通关完成
					}
				}catch(std::exception &e){
					LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
				}
				trigger->activated = true;
				trigger->activated_time = utc_now;
				trigger->times -= 1;
				forcast_triggers(trigger,utc_now);
			}
		}
		pump_triggers();
	} catch(std::exception &e){
		LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
	}
}

void Dungeon::check_triggers_all_die(){
	PROFILE_ME;

	try{
		const auto utc_now = Poseidon::get_utc_time();
		for(auto it = m_triggers.begin(); it != m_triggers.end(); ++it){
			const auto &trigger = it->second;
			if(trigger->open && !trigger->activated && (trigger->condition.type == TriggerCondition::C_ALL_DIE) && (trigger->times != 0)){
				try{
					std::istringstream iss_params(trigger->condition.params);
					auto params_array = Poseidon::JsonParser::parse_array(iss_params);
					auto type = boost::lexical_cast<std::uint64_t>(params_array.at(0).get<double>());
					if(!check_all_die(type)){
						continue;
					}
					if(type == 1){
						auto monster_removed = boost::lexical_cast<std::uint64_t>(params_array.at(1).get<double>());
						if(m_monster_removed_count < monster_removed){
							continue;
						}
					}
				}catch(std::exception &e){
					LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
				}
				trigger->activated = true;
				trigger->activated_time = utc_now;
				trigger->times -= 1;
				forcast_triggers(trigger,utc_now);
			}
		}
		pump_triggers();
	} catch(std::exception &e){
		LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
	}
}

void Dungeon::check_triggers_tag_die(){
	PROFILE_ME;

	try{
		const auto utc_now = Poseidon::get_utc_time();
		for(auto it = m_triggers.begin(); it != m_triggers.end(); ++it){
			const auto &trigger = it->second;
			if(trigger->open && !trigger->activated && (trigger->condition.type == TriggerCondition::C_TAG_DIE) && (trigger->times != 0)){
				try{
					std::istringstream iss_params(trigger->condition.params);
					auto params_array = Poseidon::JsonParser::parse_array(iss_params);
					unsigned count = 0;
					for(unsigned i = 0; i < params_array.size(); ++i){
						auto tag = boost::lexical_cast<std::string>(params_array.at(i).get<double>());
						auto it = std::find(m_die_objects.begin(),m_die_objects.end(),tag);
						if(it != m_die_objects.end()){
							count ++;
						}
					}
					if((count == 0) || (count != params_array.size())){
						continue;
					}
				}catch(std::exception &e){
					LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
				}
				trigger->activated = true;
				trigger->activated_time = utc_now;
				trigger->times -= 1;
				forcast_triggers(trigger,utc_now);
			}
		}
		pump_triggers();
	} catch(std::exception &e){
		LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
	}
}

void Dungeon::on_triggers_action(const TriggerAction &action){
	PROFILE_ME;

	switch(action.type){
	{
#define ON_TRIGGER_ACTION(trigger_action_type)	\
		}	\
		break;	\
	case (trigger_action_type): {
//=============================================================================
	ON_TRIGGER_ACTION(TriggerAction::A_CREATE_DUNGEON_OBJECT){
		//TODO
		on_triggers_create_dungeon_object(action);
	}
	ON_TRIGGER_ACTION(TriggerAction::A_DUNGEON_OBJECT_DAMAGE){
		//TODO
		on_triggers_dungeon_object_damage(action);
	}
	ON_TRIGGER_ACTION(TriggerAction::A_BUFF){
		//TODO
		on_triggers_buff(action);
	}
	ON_TRIGGER_ACTION(TriggerAction::A_SET_TRIGGER){
		//TODO
		on_triggers_set_trigger(action);
	}
	ON_TRIGGER_ACTION(TriggerAction::A_DUNGEON_FAILED){
		//TODO
		on_triggers_dungeon_failed(action);
	}
	ON_TRIGGER_ACTION(TriggerAction::A_DUNGEON_FINISHED){
		//TODO
		on_triggers_dungeon_finished(action);
	}
	ON_TRIGGER_ACTION(TriggerAction::A_SET_TASK_FINISHED){
		//TODO
		on_triggers_dungeon_task_finished(action);
	}
	ON_TRIGGER_ACTION(TriggerAction::A_SET_TASK_FAIL){
		//TODO
		on_triggers_dungeon_task_failed(action);
	}
	ON_TRIGGER_ACTION(TriggerAction::A_MOVE_CAREMA){
		//TODO
		on_triggers_dungeon_move_camera(action);
	}
	ON_TRIGGER_ACTION(TriggerAction::A_SET_SCOPE){
		//TODO
		on_triggers_dungeon_set_scope(action);
	}
	ON_TRIGGER_ACTION(TriggerAction::A_WAIT_CONFIRMATION){
		//TODO
		on_triggers_dungeon_wait_for_confirmation(action);
	}
	ON_TRIGGER_ACTION(TriggerAction::A_SHOW_PICTURE){
		//TODO
		on_triggers_dungeon_show_pictures(action);
	}
	ON_TRIGGER_ACTION(TriggerAction::A_REMOVE_PICTURE){
		//TODO
		on_triggers_dungeon_remove_pictures(action);
	}
	ON_TRIGGER_ACTION(TriggerAction::A_RANGE_DAMAGE){
		//TODO
		on_triggers_dungeon_range_damage(action);
	}
	ON_TRIGGER_ACTION(TriggerAction::A_TRANSMIT){
		//TODO
		on_triggers_dungeon_transmit(action);
	}
	ON_TRIGGER_ACTION(TriggerAction::A_SET_BLOCK){
		//TODO
		on_triggers_dungeon_set_block(action);
	}
	ON_TRIGGER_ACTION(TriggerAction::A_REMOVE_BLOCK){
		//TODO
		on_triggers_dungeon_remove_block(action);
	}
	ON_TRIGGER_ACTION(TriggerAction::A_PAUSE){
		//TODO
		on_triggers_dungeon_pause_fight(action);
	}
	ON_TRIGGER_ACTION(TriggerAction::A_RESTART){
		//TODO
		on_triggers_dungeon_restart_fight(action);
	}
	ON_TRIGGER_ACTION(TriggerAction::A_HIDE_ALL){
		//TODO
		on_triggers_dungeon_hide_all_solider(action);
	}
	ON_TRIGGER_ACTION(TriggerAction::A_UNHIDE_ALL){
		//TODO
		on_triggers_dungeon_unhide_all_solider(action);
	}
	ON_TRIGGER_ACTION(TriggerAction::A_HIDE_COORDS){
		//TODO
		on_triggers_dungeon_hide_coords(action);
	}
	ON_TRIGGER_ACTION(TriggerAction::A_UNHIDE_COORDS){
		//TODO
		on_triggers_dungeon_unhide_coords(action);
	}
	ON_TRIGGER_ACTION(TriggerAction::A_DEFENSE_MATRIX){
		//TODO
		on_triggers_dungeon_defense_matrix(action);
	}
	ON_TRIGGER_ACTION(TriggerAction::A_SET_FOOT_ANNIMATION){
		//TODO
		on_triggers_dungeon_set_foot_annimation(action);
	}
//=============================================================================
#undef ON_TRIGGER_ACTION
		}
		break;
	default:
		LOG_EMPERY_DUNGEON_WARNING("Unknown trigger action type = ", static_cast<unsigned>(action.type));
		break;
	}
}

void Dungeon::on_triggers_create_dungeon_object(const TriggerAction &action){
	PROFILE_ME;

	try{
		if(action.type != TriggerAction::A_CREATE_DUNGEON_OBJECT){
			return;
		}
		std::istringstream iss_effect_param(action.params);
		auto effect_param_array = Poseidon::JsonParser::parse_array(iss_effect_param);
		const auto dungeon_client = get_dungeon_client();
		for(auto it = effect_param_array.begin(); it != effect_param_array.end(); ++it){
			auto &elem = it->get<Poseidon::JsonArray>();
			if(elem.size() != 4){
				LOG_EMPERY_DUNGEON_WARNING("create dungeon object params error = ");
				continue;
			}
			if(dungeon_client){
				try {
					auto &birth                         = elem.at(1).get<Poseidon::JsonArray>();
					auto birth_x                        = static_cast<int>(birth.at(0).get<double>());
					auto birth_y                        = static_cast<int>(birth.at(1).get<double>());
					Coord default_birth_coord(birth_x,birth_y);
					Coord valid_birth_coord(0,0);
					if(!get_monster_birth_coord(default_birth_coord,valid_birth_coord)){
						LOG_EMPERY_DUNGEON_WARNING("server can not find a valid for monster, default_birth_coord = ",default_birth_coord);
						return;
					}
					Msg::DS_DungeonCreateMonster msgCreateMonster;
					msgCreateMonster.dungeon_uuid       = get_dungeon_uuid().str();
					msgCreateMonster.map_object_type_id = static_cast<std::uint64_t>(elem.at(0).get<double>());
					msgCreateMonster.x                  = valid_birth_coord.x();
					msgCreateMonster.y                  = valid_birth_coord.y();
					auto &dest                          = elem.at(2).get<Poseidon::JsonArray>();
					msgCreateMonster.dest_x             = static_cast<int>(dest.at(0).get<double>());
					msgCreateMonster.dest_y             = static_cast<int>(dest.at(1).get<double>());
					msgCreateMonster.tag                =  boost::lexical_cast<std::string>(elem.at(3).get<double>());
					dungeon_client->send(msgCreateMonster);
					LOG_EMPERY_DUNGEON_DEBUG("msg create monster:",msgCreateMonster);
				} catch(std::exception &e){
					LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
					dungeon_client->shutdown(e.what());
				}
			}
		}
	} catch(std::exception &e){
		LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
	}
}

void Dungeon::on_triggers_dungeon_object_damage(const TriggerAction &action){
	PROFILE_ME;

	try{
		if(action.type != TriggerAction::A_DUNGEON_OBJECT_DAMAGE){
			return;
		}
		const auto utc_now = Poseidon::get_utc_time();
		std::istringstream iss_effect_param(action.params);
		auto effect_param_array = Poseidon::JsonParser::parse_array(iss_effect_param);
		if(effect_param_array.size() < 5){
			LOG_EMPERY_DUNGEON_WARNING("trigger dungeon object damage parmas is error");
		}
		bool is_target_monster = effect_param_array.at(0).get<double>();
		auto loop              = static_cast<std::uint64_t>(effect_param_array.at(1).get<double>());
		auto damage            = static_cast<std::uint64_t>(effect_param_array.at(2).get<double>());
		auto delay             = static_cast<std::uint64_t>(effect_param_array.at(3).get<double>());
		std::vector<Coord> damage_range;
		for(unsigned i = 5; i < effect_param_array.size(); ++i){
			auto &elem = effect_param_array.at(i).get<Poseidon::JsonArray>();
			auto x = static_cast<int>(elem.at(0).get<double>());
			auto y = static_cast<int>(elem.at(1).get<double>());
			damage_range.push_back(Coord(x,y));
		}
		auto trigger_damage = boost::make_shared<TriggerDamage>(is_target_monster,loop,damage,delay,utc_now,std::move(damage_range));
		m_triggers_damages.push_back(std::move(trigger_damage));
	} catch(std::exception &e){
			LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
	}
}

void Dungeon::do_triggers_damage(const boost::shared_ptr<TriggerDamage>& trigger_damage){
	PROFILE_ME;

	try{
		const auto dungeon_client = get_dungeon_client();
		if(!dungeon_client){
			return;
		}
		auto & coords = trigger_damage->damage_range;
		std::vector<boost::shared_ptr<DungeonObject>> ret;
		get_objects_all(ret);
		for(auto it = coords.begin(); it != coords.end(); ++it){
			auto &coord = *it;
			for(auto itt = ret.begin(); itt != ret.end(); ++itt){
				if((trigger_damage->is_target_monster == (*itt)->is_monster()) && (coord == (*itt)->get_coord())){
					try {
						Msg::DS_DungeonObjectAttackAction msg;
						msg.dungeon_uuid = get_dungeon_uuid().str();
						/*
						msg.attacking_account_uuid = get_owner_uuid().str();
						msg.attacking_object_uuid = get_dungeon_object_uuid().str();
						msg.attacking_object_type_id = get_dungeon_object_type_id().get();
						msg.attacking_coord_x = coord.x();
						msg.attacking_coord_y = coord.y();
						*/
						msg.attacked_account_uuid = (*itt)->get_owner_uuid().str();
						msg.attacked_object_uuid  = (*itt)->get_dungeon_object_uuid().str();
						msg.attacked_object_type_id = (*itt)->get_dungeon_object_type_id().get();
						msg.attacked_coord_x = (*itt)->get_coord().x();
						msg.attacked_coord_y = (*itt)->get_coord().y();
						msg.result_type = 1;
						msg.soldiers_damaged = trigger_damage->damage;
						auto sresult = dungeon_client->send_and_wait(msg);
						if(sresult.first != Poseidon::Cbpp::ST_OK){
							LOG_EMPERY_DUNGEON_DEBUG("trigger damage fail", sresult.first, ", msg = ", sresult.second);
							continue;
						}
						LOG_EMPERY_DUNGEON_DEBUG("trigger damage attack action:",msg);
					} catch(std::exception &e){
						LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
						dungeon_client->shutdown(e.what());
					}
				}
			}
		}
	} catch(std::exception &e){
			LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
	}
}

void Dungeon::on_triggers_buff(const TriggerAction &action){
	PROFILE_ME;

	try{
		if(action.type != TriggerAction::A_BUFF){
			return;
		}
		std::istringstream iss_effect_param(action.params);
		auto effect_param_array = Poseidon::JsonParser::parse_array(iss_effect_param);
		const auto dungeon_client = get_dungeon_client();
		if(effect_param_array.size() != 2){
			LOG_EMPERY_DUNGEON_WARNING("trigger buff params error = ");
			return;
		}
		const auto create_uuid = DungeonObjectUuid(action.dungeon_params);
		auto dungeon_object = get_object(create_uuid);
		if(!dungeon_object){
			LOG_EMPERY_DUNGEON_WARNING("trigger buff dungeon object is null");
			return;
		}
		auto create_owner_uuid = dungeon_object->get_owner_uuid();
		auto buff_type_id = static_cast<std::uint64_t>(effect_param_array.at(0).get<double>());
		auto &birth_coord  = effect_param_array.at(1).get<Poseidon::JsonArray>();
		auto birth_x                        = static_cast<int>(birth_coord.at(0).get<double>());
		auto birth_y                        = static_cast<int>(birth_coord.at(1).get<double>());
		if(dungeon_client){
			try {
				//副本buff
				Msg::DS_DungeonCreateBuff msg;
				msg.dungeon_uuid       = get_dungeon_uuid().str();
				msg.buff_type_id       = buff_type_id;
				msg.x                  = birth_x;
				msg.y                  = birth_y;
				msg.create_uuid        = create_uuid.str();
				msg.create_owner_uuid  = create_owner_uuid.str();
				dungeon_client->send(msg);

				//将buff直接加到野怪或者兵身上
				const auto data_buff = Data::DungeonBuff::require(DungeonBuffTypeId(buff_type_id));
				if(data_buff->target == 1){
					Msg::DS_DungeonObjectAddBuff msg;
					msg.dungeon_uuid        = get_dungeon_uuid().str();
					msg.dungeon_object_uuid = create_uuid.str();
					msg.buff_type_id        = buff_type_id;
					dungeon_client->send(msg);
				}else if(data_buff->target == 2){
					std::vector<boost::shared_ptr<DungeonObject>> ret;
					get_dungeon_objects_by_account(ret,create_owner_uuid);
					for(auto it = ret.begin(); it != ret.end();++it){
						auto &friend_object = *it;
						try{
							Msg::DS_DungeonObjectAddBuff msg;
							msg.dungeon_uuid        = get_dungeon_uuid().str();
							msg.dungeon_object_uuid = friend_object->get_dungeon_object_uuid().str();
							msg.buff_type_id        = buff_type_id;
							dungeon_client->send(msg);
						} catch(std::exception &e){
							LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
						}
					}
				}
			} catch(std::exception &e){
				LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
				dungeon_client->shutdown(e.what());
			}
		}
	} catch(std::exception &e){
		LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
	}
}
void Dungeon::on_triggers_set_trigger(const TriggerAction &action){
	PROFILE_ME;

	try{
		if(action.type != TriggerAction::A_SET_TRIGGER){
			return;
		}
		const auto utc_now = Poseidon::get_utc_time();
		std::istringstream iss_param(action.params);
		auto param_array = Poseidon::JsonParser::parse_array(iss_param);
		std::uint64_t trigger_id = static_cast<std::uint64_t>(param_array.at(0).get<double>());
		std::uint64_t open = static_cast<std::uint64_t>(param_array.at(1).get<double>());
		auto it = m_triggers.find(trigger_id);
		if(it != m_triggers.end()){
			const auto &trigger = it->second;
			trigger->open = open;
			//触发器触发器直接激活1次
			if(open && trigger->condition.type == TriggerCondition::C_TRIGGER){
				if(!trigger->activated&&(trigger->times != 0)){
					trigger->activated_time = utc_now;
					trigger->open = true;
					trigger->activated = open;
					trigger->times -= 1;
					forcast_triggers(trigger,utc_now);
				}
			}
		}
	} catch(std::exception &e){
				LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
	}
}

void Dungeon::on_triggers_dungeon_failed(const TriggerAction &action){
	PROFILE_ME;

	try{
		if(action.type != TriggerAction::A_DUNGEON_FAILED){
			return;
		}
		if(m_dungeon_state != S_INIT){
			LOG_EMPERY_DUNGEON_DEBUG("on trigger dungeon fail,but the dungeon have finished,currecnt state = ",m_dungeon_state);
			return;
		}
		m_dungeon_state = S_FAIL;
		const auto dungeon_client = get_dungeon_client();
		if(dungeon_client){
			try {
				Msg::DS_DungeonPlayerLoses msgDungeonLoss;
				msgDungeonLoss.dungeon_uuid       = get_dungeon_uuid().str();
				msgDungeonLoss.account_uuid       = get_founder_uuid().str();
				dungeon_client->send(msgDungeonLoss);
				LOG_EMPERY_DUNGEON_DEBUG("msg dungeon expired:",msgDungeonLoss);
			} catch(std::exception &e){
				LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
				dungeon_client->shutdown(e.what());
			}
		}
	} catch(std::exception &e){
		LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
	}
}

void Dungeon::on_triggers_dungeon_finished(const TriggerAction &action){
	PROFILE_ME;

	try{
		if(action.type != TriggerAction::A_DUNGEON_FINISHED){
			return;
		}
		if(m_dungeon_state != S_INIT){
			LOG_EMPERY_DUNGEON_DEBUG("on trigger dungeon finished,but the dungeon have finished,currecnt state = ",m_dungeon_state);
			return;
		}
		m_dungeon_state = S_PASS;
		check_triggers_dungeon_finish();
		const auto dungeon_client = get_dungeon_client();
		if(dungeon_client){
			try {
				Msg::DS_DungeonPlayerWins msgDungeonWin;
				msgDungeonWin.dungeon_uuid       = get_dungeon_uuid().str();
				msgDungeonWin.account_uuid       = get_founder_uuid().str();
				for(auto it = m_finish_tasks.begin(); it != m_finish_tasks.end(); ++it){
					auto &task_finished = *msgDungeonWin.tasks_finished.emplace(msgDungeonWin.tasks_finished.end());
					task_finished.dungeon_task_id = *it;
				}
				for(auto it = m_damage_solider.begin(); it != m_damage_solider.end();++it){
					auto &damage_solider = *msgDungeonWin.damage_solider.emplace(msgDungeonWin.damage_solider.end());
					damage_solider.dungeon_object_type_id = it->first.get();
					damage_solider.count                    = it->second;
				}
				msgDungeonWin.total_damage_solider = get_total_damage_solider();
				dungeon_client->send(msgDungeonWin);
				LOG_EMPERY_DUNGEON_DEBUG("msg dungeon win:",msgDungeonWin);
			} catch(std::exception &e){
				LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
				dungeon_client->shutdown(e.what());
			}
		}
	} catch(std::exception &e){
		LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
	}
}

void Dungeon::on_triggers_dungeon_task_finished(const TriggerAction &action){
	PROFILE_ME;

	try{
		if(action.type != TriggerAction::A_SET_TASK_FINISHED){
			return;
		}
		std::istringstream iss_param(action.params);
		auto param_array = Poseidon::JsonParser::parse_array(iss_param);
		for(unsigned i = 0; i < param_array.size(); ++i){
			auto task_id = static_cast<std::uint64_t>(param_array.at(i).get<double>());
			auto it  = std::find(m_finish_tasks.begin(),m_finish_tasks.end(), task_id);
			if(it == m_finish_tasks.end()){
				m_finish_tasks.push_back(task_id);
			}
		}
	} catch(std::exception &e){
		LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
	}
}

void Dungeon::on_triggers_dungeon_task_failed(const TriggerAction &action){
	PROFILE_ME;
}

void Dungeon::on_triggers_dungeon_move_camera(const TriggerAction &action){
	PROFILE_ME;

	try{
		if(action.type != TriggerAction::A_MOVE_CAREMA){
			return;
		}
		std::istringstream iss_param(action.params);
		auto param_array = Poseidon::JsonParser::parse_array(iss_param);
		auto &dest = param_array.at(0).get<Poseidon::JsonArray>();
		auto x = static_cast<int>(dest.at(0).get<double>());
		auto y = static_cast<int>(dest.at(1).get<double>());
		auto duration = static_cast<std::uint64_t>(param_array.at(1).get<double>());
		auto pos = static_cast<std::uint64_t>(param_array.at(2).get<double>());
		const auto dungeon_client = get_dungeon_client();
		if(dungeon_client){
			try {
				Msg::DS_DungeonMoveCamera msgDungeonMoveCamera;
				msgDungeonMoveCamera.dungeon_uuid       = get_dungeon_uuid().str();
				msgDungeonMoveCamera.x                  = x;
				msgDungeonMoveCamera.y                  = y;
				msgDungeonMoveCamera.movement_duration  = duration;
				msgDungeonMoveCamera.position_type      = pos;
				dungeon_client->send(msgDungeonMoveCamera);
				LOG_EMPERY_DUNGEON_DEBUG("msg dungeon move camera:",msgDungeonMoveCamera);
			} catch(std::exception &e){
				LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
				dungeon_client->shutdown(e.what());
			}
		}
	} catch(std::exception &e){
		LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
	}
}

void Dungeon::on_triggers_dungeon_set_scope(const TriggerAction &action){
	PROFILE_ME;

	try{
		if(action.type != TriggerAction::A_SET_SCOPE){
			return;
		}
		std::istringstream iss_param(action.params);
		auto param_array = Poseidon::JsonParser::parse_array(iss_param);
		auto &dest = param_array.at(0).get<Poseidon::JsonArray>();
		auto x = static_cast<int>(dest.at(0).get<double>());
		auto y = static_cast<int>(dest.at(1).get<double>());
		auto width = static_cast<std::uint64_t>(param_array.at(1).get<double>());
		auto height = static_cast<std::uint64_t>(param_array.at(2).get<double>());
		const auto dungeon_client = get_dungeon_client();
		if(dungeon_client){
			try {
				Msg::DS_DungeonSetScope msgDungeonSetScope;
				msgDungeonSetScope.dungeon_uuid       = get_dungeon_uuid().str();
				msgDungeonSetScope.x                  = x;
				msgDungeonSetScope.y                  = y;
				msgDungeonSetScope.width              = width;
				msgDungeonSetScope.height             = height;
				dungeon_client->send(msgDungeonSetScope);
				LOG_EMPERY_DUNGEON_DEBUG("msg dungeon set scope:",msgDungeonSetScope);
			} catch(std::exception &e){
				LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
				dungeon_client->shutdown(e.what());
			}
		}
	} catch(std::exception &e){
		LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
	}
}

void Dungeon::on_triggers_dungeon_wait_for_confirmation(const TriggerAction &action){
	PROFILE_ME;

	try{
		if(action.type != TriggerAction::A_WAIT_CONFIRMATION){
			return;
		}
		std::istringstream iss_param(action.params);
		auto param_array = Poseidon::JsonParser::parse_array(iss_param);
		auto context =  boost::lexical_cast<std::string>(param_array.at(0).get<double>());
		auto type    =  static_cast<unsigned>(param_array.at(1).get<double>());
		auto param1  =  static_cast<int>(param_array.at(2).get<double>());
		auto param2  =  static_cast<int>(param_array.at(3).get<double>());
		auto param3  =  boost::lexical_cast<std::string>(param_array.at(4).get<double>());
		auto trigger_id = static_cast<std::uint64_t>(param_array.at(5).get<double>());

		auto it = m_triggers_confirmation.find(context);
		if(it != m_triggers_confirmation.end()){
			LOG_EMPERY_DUNGEON_WARNING("trigger same confirmation at the same time, confirmation = ", context);
			return;
		}
		auto trigger_confirmation = boost::make_shared<TriggerConfirmation>(context,type,param1,param2,param3,trigger_id,false);
		const auto result = m_triggers_confirmation.emplace(context, std::move(trigger_confirmation));
		if(!result.second){
			LOG_EMPERY_DUNGEON_WARNING("Dungeon trigger confirmation already exists: context = ", context);
			DEBUG_THROW(Exception, sslit("Dungeon trigger confirmation already exists"));
		}
		const auto dungeon_client = get_dungeon_client();
		if(dungeon_client){
			try {
				Msg::DS_DungeonWaitForPlayerConfirmation msgDungeonWaitForPlayerConfirm;
				msgDungeonWaitForPlayerConfirm.dungeon_uuid       = get_dungeon_uuid().str();
				msgDungeonWaitForPlayerConfirm.context            = context;
				msgDungeonWaitForPlayerConfirm.type               = type;
				msgDungeonWaitForPlayerConfirm.param1             = param1;
				msgDungeonWaitForPlayerConfirm.param2             = param2;
				msgDungeonWaitForPlayerConfirm.param3             = param3;
				dungeon_client->send(msgDungeonWaitForPlayerConfirm);
				LOG_EMPERY_DUNGEON_DEBUG("msg dungeon wait for player confirmation:",msgDungeonWaitForPlayerConfirm);
			} catch(std::exception &e){
				LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
				dungeon_client->shutdown(e.what());
			}
		}
	} catch(std::exception &e){
		LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
	}
}

void Dungeon::on_triggers_dungeon_player_confirmation(std::string context){
	PROFILE_ME;
	try{
		auto it = m_triggers_confirmation.find(context);
		if(it == m_triggers_confirmation.end()){
			LOG_EMPERY_DUNGEON_WARNING("player confirmation can't find, context = ",context);
			return;
		}
		const auto utc_now = Poseidon::get_utc_time();
		auto &trigger_confirmation = it->second;
		auto trigger_id = trigger_confirmation->trigger_id;
		auto itt = m_triggers.find(trigger_id);
		if(itt != m_triggers.end()){
			const auto &trigger = itt->second;
			if(trigger->activated){
				LOG_EMPERY_DUNGEON_WARNING("trigger have be activated, trigger_id = ", trigger_id);
			}else{
				trigger->activated_time = utc_now;
				trigger->activated = true;
				forcast_triggers(trigger,utc_now);
			}
		}else{
			LOG_EMPERY_DUNGEON_WARNING("can't find the trigger, trigger_id = ", trigger_id);
		}
		m_triggers_confirmation.erase(it);
	} catch(std::exception &e){
		LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
	}
}

void Dungeon::notify_triggers_executive(const boost::shared_ptr<Trigger> &trigger){
	if(!trigger){
		return;
	}
	const auto dungeon_client = get_dungeon_client();
	if(dungeon_client){
		try{
			Msg::DS_DungeonTriggerEffectExecutive msg;
			msg.dungeon_uuid    = get_dungeon_uuid().str();
			msg.trigger_id      = trigger->trigger_id;
			for(auto it = trigger->actions.begin(); it != trigger->actions.end(); ++it){
				auto &effects = *msg.effects.emplace(msg.effects.end());
				effects.effect_type = it->type;
			}
			dungeon_client->send(msg);
		}
		catch(std::exception &e){
			LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
			dungeon_client->shutdown(e.what());
		}
	}
}

void Dungeon::on_triggers_dungeon_show_pictures(const TriggerAction &action){
	PROFILE_ME;

	try{
		if(action.type != TriggerAction::A_SHOW_PICTURE){
			return;
		}
		std::istringstream iss_param(action.params);
		auto param_array = Poseidon::JsonParser::parse_array(iss_param);
		if(param_array.size() != 7){
			LOG_EMPERY_DUNGEON_WARNING("dungeon show pictures param error,size != 3",action.params);
			return;
		}
		auto picture_url =  param_array.at(0).get<std::string>();
		auto id     =  static_cast<std::uint64_t>(param_array.at(1).get<double>());
		auto type   =  static_cast<int>(param_array.at(2).get<double>());
		auto layer  =  static_cast<int>(param_array.at(3).get<double>());
		auto tween  =  static_cast<int>(param_array.at(4).get<double>());
		auto time   =  static_cast<int>(param_array.at(5).get<double>());
		auto &picture_coord  =  param_array.at(6).get<Poseidon::JsonArray>();
		auto x = static_cast<int>(picture_coord.at(0).get<double>()); 
		auto y = static_cast<int>(picture_coord.at(1).get<double>());

		const auto dungeon_client = get_dungeon_client();
		if(dungeon_client){
			try {
				Msg::DS_DungeonShowPicture msg;
				msg.dungeon_uuid    = get_dungeon_uuid().str();
				msg.picture_url     = picture_url;
				msg.picture_id      = id;
				msg.type            = type;
				msg.layer           = layer;
				msg.tween           = tween;
				msg.time            = time;
				msg.x               = x;
				msg.y               = y;
				dungeon_client->send(msg);
			} catch(std::exception &e){
				LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
				dungeon_client->shutdown(e.what());
			}
		}
	} catch(std::exception &e){
		LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
	}
}
void Dungeon::on_triggers_dungeon_remove_pictures(const TriggerAction &action){
	PROFILE_ME;

	try{
		if(action.type != TriggerAction::A_REMOVE_PICTURE){
			return;
		}
		std::istringstream iss_param(action.params);
		auto param_array = Poseidon::JsonParser::parse_array(iss_param);
		if(param_array.size() != 3){
			LOG_EMPERY_DUNGEON_WARNING("dungeon remove pictures param error,size != 1",action.params);
			return;
		}
		auto id     =  static_cast<std::uint64_t>(param_array.at(0).get<double>());
		auto tween  =  static_cast<int>(param_array.at(1).get<double>());
		auto time   =  static_cast<int>(param_array.at(2).get<double>());
		const auto dungeon_client = get_dungeon_client();
		if(dungeon_client){
			try {
				Msg::DS_DungeonRemovePicture msg;
				msg.dungeon_uuid    = get_dungeon_uuid().str();
				msg.picture_id      = id;
				msg.tween           = tween;
				msg.time            = time;
				dungeon_client->send(msg);
			} catch(std::exception &e){
				LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
				dungeon_client->shutdown(e.what());
			}
		}
	} catch(std::exception &e){
		LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
	}
}

void Dungeon::on_triggers_dungeon_range_damage(const TriggerAction &action){
	PROFILE_ME;

	const auto dungeon_client = get_dungeon_client();
	try{
		if(action.type != TriggerAction::A_RANGE_DAMAGE){
			return;
		}
		std::istringstream iss_param(action.params);
		auto param_array = Poseidon::JsonParser::parse_array(iss_param);
		if(param_array.size() != 2){
			LOG_EMPERY_DUNGEON_WARNING("dungeon range damage error,size != 1",action.params);
			return;
		}
		auto trap_coord = param_array.at(0).get<Poseidon::JsonArray>();
		auto x = static_cast<int>(trap_coord.at(0).get<double>());
		auto y = static_cast<int>(trap_coord.at(1).get<double>());
		auto id  =  static_cast<std::uint64_t>(param_array.at(1).get<double>());
		double k = 0.03;
		const auto trap_data = Data::DungeonTrap::require(DungeonTrapTypeId(id));
		std::vector<Coord> surrounding;
		for(unsigned i = 0; i <= trap_data->attack_range;++i){
			get_surrounding_coords(surrounding,Coord(x,y),i);
		}
		std::vector<boost::shared_ptr<DungeonObject>> ret;
		get_objects_all(ret);
		for(auto it = surrounding.begin(); it != surrounding.end();++it){
			auto &coord = *it;
			for(auto itt = ret.begin(); itt != ret.end(); ++itt){
				const auto target_object = *itt;
				if(coord == target_object->get_coord()){
					try {
						//伤害计算
						double relative_rate = Data::DungeonObjectRelative::get_relative(trap_data->attack_type,target_object->get_arm_defence_type());
						double total_defense = target_object->get_total_defense();
						double damage = trap_data->attack * 1 * relative_rate * (1 - k *total_defense/(1 + k*total_defense));
						double defense_matrix = get_dungeon_attribute(target_object->get_dungeon_object_uuid(),target_object->get_owner_uuid(),EmperyCenter::AttributeIds::ID_DEFENSE_MATRIX) / 1000.0;
						if(defense_matrix > 0.0){
							damage = damage*(1 - defense_matrix);
							LOG_EMPERY_DUNGEON_FATAL("defense matrix damage:",damage);
						}
						Msg::DS_DungeonObjectAttackAction msg;
						msg.dungeon_uuid = get_dungeon_uuid().str();
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
						auto sresult = dungeon_client->send_and_wait(msg);
						if(sresult.first != Poseidon::Cbpp::ST_OK){
							LOG_EMPERY_DUNGEON_DEBUG("trap damge fail", sresult.first, ", msg = ", sresult.second);
							continue;
						}
						LOG_EMPERY_DUNGEON_DEBUG("trap damage attack action:",msg);
					} catch(std::exception &e){
						LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
						dungeon_client->shutdown(e.what());
					}
				}
			}
		}
	} catch(std::exception &e){
		LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
		dungeon_client->shutdown(e.what());
	}
}

void Dungeon::on_triggers_dungeon_transmit(const TriggerAction &action){
	PROFILE_ME;

	const auto dungeon_client = get_dungeon_client();
	try{
		if(action.type != TriggerAction::A_TRANSMIT){
			return;
		}
		const auto create_uuid = DungeonObjectUuid(action.dungeon_params);
		auto dungeon_object = get_object(create_uuid);
		if(!dungeon_object){
			LOG_EMPERY_DUNGEON_WARNING("on triggers dungeon transmit no dungeon object");
			return;
		}
		std::istringstream iss_param(action.params);
		auto param_array = Poseidon::JsonParser::parse_array(iss_param);
		if(param_array.size() != 2){
			LOG_EMPERY_DUNGEON_WARNING("dungeon range damage error,size != 1",action.params);
			return;
		}
		auto x = static_cast<int>(param_array.at(0).get<double>());
		auto y = static_cast<int>(param_array.at(1).get<double>());
		auto origin   = Coord(x,y);
		std::vector<Coord> surrounding;
		for(unsigned i = 1; i <= 5;++i){
			get_surrounding_coords(surrounding,origin,i);
		}
		//std::deque<std::pair<signed char, signed char>> waypoints;
		bool create_valid_coord = check_valid_coord_for_birth(origin);
		if(create_valid_coord){
			dungeon_object->set_action(origin, {}, DungeonObject::ACT_GUARD,"");
		}
		std::vector<boost::shared_ptr<DungeonObject>> ret;
		get_dungeon_objects_by_account(ret,dungeon_object->get_owner_uuid());
		//通知传送
		Msg::DS_DungeonObjectPrepareTransmit msg;
		msg.dungeon_uuid    = get_dungeon_uuid().str();
		for(auto it = ret.begin(); it != ret.end(); ++it){
			auto &friend_dungeon_object = *it;
			auto &transmit_info = *msg.transmit_objects.emplace(msg.transmit_objects.end());
			transmit_info.object_uuid = friend_dungeon_object->get_dungeon_object_uuid().str();
			transmit_info.x           = friend_dungeon_object->get_coord().x();
			transmit_info.y           = friend_dungeon_object->get_coord().y();
		}
		if(dungeon_client){
			dungeon_client->send(msg);
		}
		//执行传送
		unsigned i = 0;
		for(auto it = ret.begin(); it != ret.end(); ++it){
			auto &friend_dungeon_object = *it;
			if((friend_dungeon_object->get_dungeon_object_uuid() == create_uuid) && create_valid_coord){
				continue;
			}
			bool find_valid_coord = false;
			for(;i < surrounding.size(); ++ i){
				auto &coord = surrounding.at(i);
				bool valid_coord = check_valid_coord_for_birth(coord);
				if(valid_coord){
					friend_dungeon_object->set_action(coord, {}, DungeonObject::ACT_GUARD,"");
					find_valid_coord = true;
					break;
				}else{
					continue;
				}
			}
			if(!find_valid_coord){
				LOG_EMPERY_DUNGEON_WARNING("dungeon object transmit fail,can not find a valid coord to transmit,dungeon_object_uuid:",friend_dungeon_object->get_dungeon_object_uuid());
			}
		}
	} catch(std::exception &e){
		LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
		dungeon_client->shutdown(e.what());
	}
}

void Dungeon::on_triggers_dungeon_set_block(const TriggerAction &action){
	PROFILE_ME;

	const auto dungeon_client = get_dungeon_client();
	try{
		if(action.type != TriggerAction::A_SET_BLOCK){
			return;
		}
		std::istringstream iss_param(action.params);
		auto param_array = Poseidon::JsonParser::parse_array(iss_param);
		Msg::DS_DungeonCreateBlocks msg;
		msg.dungeon_uuid = get_dungeon_uuid().str();
		for(unsigned i = 0; i < param_array.size(); ++i){
			auto block_coord  = param_array.at(i).get<Poseidon::JsonArray>();
			auto x = static_cast<int>(block_coord.at(0).get<double>());
			auto y = static_cast<int>(block_coord.at(1).get<double>());
			Coord temp_coord = Coord(x,y);
			m_dungeon_blocks.insert(std::move(temp_coord));
			auto &blocks = *msg.blocks.emplace(msg.blocks.end());
			blocks.x = x;
			blocks.y = y;
		}
		dungeon_client->send(msg);
	} catch(std::exception &e){
		LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
		dungeon_client->shutdown(e.what());
	}
}

void Dungeon::on_triggers_dungeon_remove_block(const TriggerAction &action){
	PROFILE_ME;

	const auto dungeon_client = get_dungeon_client();
	try{
		if(action.type != TriggerAction::A_REMOVE_BLOCK){
			return;
		}
		std::istringstream iss_param(action.params);
		auto param_array = Poseidon::JsonParser::parse_array(iss_param);
		Msg::DS_DungeonRemoveBlocks msg;
		msg.dungeon_uuid = get_dungeon_uuid().str();
		for(unsigned i = 0; i < param_array.size(); ++i){
			auto block_coord  = param_array.at(i).get<Poseidon::JsonArray>();
			auto x = static_cast<int>(block_coord.at(0).get<double>());
			auto y = static_cast<int>(block_coord.at(1).get<double>());
			m_dungeon_blocks.erase(Coord(x,y));
			auto &blocks = *msg.blocks.emplace(msg.blocks.end());
			blocks.x = x;
			blocks.y = y;
		}
		dungeon_client->send(msg);
	} catch(std::exception &e){
		LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
		dungeon_client->shutdown(e.what());
	}
}

void Dungeon::on_triggers_dungeon_pause_fight(const TriggerAction &action){
	PROFILE_ME;

	try{
		if(action.type != TriggerAction::A_PAUSE){
			return;
		}
		set_fight_state(FIGHT_PAUSE);
	} catch(std::exception &e){
		LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
	}
}
void Dungeon::on_triggers_dungeon_restart_fight(const TriggerAction &action){
	PROFILE_ME;

	try{
		if(action.type != TriggerAction::A_RESTART){
			return;
		}
		set_fight_state(FIGHT_START);
	} catch(std::exception &e){
		LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
	}
}

void Dungeon::on_triggers_dungeon_hide_all_solider(const TriggerAction &action){
	PROFILE_ME;

	const auto dungeon_client = get_dungeon_client();
	try{
		if(action.type != TriggerAction::A_HIDE_ALL){
			return;
		}
		std::istringstream iss_param(action.params);
		auto param_array = Poseidon::JsonParser::parse_array(iss_param);
		if(param_array.size() != 1){
			LOG_EMPERY_DUNGEON_WARNING("dungeon hide all solider params error,size != 1",action.params);
			return;
		}
		auto type = static_cast<std::uint64_t>(param_array.at(0).get<double>());
		Msg::DS_DungeonHideSolider msg;
		msg.dungeon_uuid = get_dungeon_uuid().str();
		msg.type         = type;
		dungeon_client->send(msg);
	} catch(std::exception &e){
		LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
	}
}

void Dungeon::on_triggers_dungeon_unhide_all_solider(const TriggerAction &action){
	PROFILE_ME;

	const auto dungeon_client = get_dungeon_client();
	try{
		if(action.type != TriggerAction::A_UNHIDE_ALL){
			return;
		}
		std::istringstream iss_param(action.params);
		auto param_array = Poseidon::JsonParser::parse_array(iss_param);
		if(param_array.size() != 1){
			LOG_EMPERY_DUNGEON_WARNING("dungeon unhide all params error,size != 1",action.params);
			return;
		}
		auto type = static_cast<std::uint64_t>(param_array.at(0).get<double>());
		Msg::DS_DungeonUnhideSolider msg;
		msg.dungeon_uuid = get_dungeon_uuid().str();
		msg.type         = type;
		dungeon_client->send(msg);
	} catch(std::exception &e){
		LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
	}
}

void Dungeon::on_triggers_dungeon_hide_coords(const TriggerAction &action){
	PROFILE_ME;

	const auto dungeon_client = get_dungeon_client();
	try{
		if(action.type != TriggerAction::A_HIDE_COORDS){
			return;
		}
		std::istringstream iss_param(action.params);
		auto param_array = Poseidon::JsonParser::parse_array(iss_param);
		if(param_array.empty()){
			LOG_EMPERY_DUNGEON_WARNING("dungeon hide coord empty,error params",action.params);
			return;
		}
		Msg::DS_DungeonHideCoords msg;
		msg.dungeon_uuid = get_dungeon_uuid().str();
		for(unsigned i = 0; i < param_array.size(); ++i){
			auto temp_coord  = param_array.at(i).get<Poseidon::JsonArray>();
			auto &hide_coord = *msg.hide_coord.emplace(msg.hide_coord.end());
			hide_coord.x = static_cast<int>(temp_coord.at(0).get<double>());
			hide_coord.y = static_cast<int>(temp_coord.at(1).get<double>());
		}
		dungeon_client->send(msg);
	} catch(std::exception &e){
		LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
	}
}

void Dungeon::on_triggers_dungeon_unhide_coords(const TriggerAction &action){
	PROFILE_ME;

	const auto dungeon_client = get_dungeon_client();
	try{
		if(action.type != TriggerAction::A_UNHIDE_COORDS){
			return;
		}
		std::istringstream iss_param(action.params);
		auto param_array = Poseidon::JsonParser::parse_array(iss_param);
		if(param_array.empty()){
			LOG_EMPERY_DUNGEON_WARNING("dungeon unhide coord empty,error params",action.params);
			return;
		}
		Msg::DS_DungeonUnhideCoords msg;
		msg.dungeon_uuid = get_dungeon_uuid().str();
		for(unsigned i = 0; i < param_array.size(); ++i){
			auto temp_coord  = param_array.at(i).get<Poseidon::JsonArray>();
			auto &unhide_coord = *msg.unhide_coord.emplace(msg.unhide_coord.end());
			unhide_coord.x = static_cast<int>(temp_coord.at(0).get<double>());
			unhide_coord.y = static_cast<int>(temp_coord.at(1).get<double>());
		}
		dungeon_client->send(msg);
	} catch(std::exception &e){
		LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
	}
}

void Dungeon::on_triggers_dungeon_defense_matrix(const TriggerAction &action){
	PROFILE_ME;

	const auto dungeon_client = get_dungeon_client();
	try{
		if(action.type != TriggerAction::A_DEFENSE_MATRIX){
			return;
		}
		std::istringstream iss_param(action.params);
		auto param_array = Poseidon::JsonParser::parse_array(iss_param);
		if(param_array.size() != 2){
			LOG_EMPERY_DUNGEON_WARNING("dungeon defend matrix size != 2,error params",action.params);
			return;
		}
		auto tags  = param_array.at(0).get<Poseidon::JsonArray>();
		std::vector<std::string> defense_matrix_vec;
		for(unsigned i = 0; i < tags.size(); ++i){
			std::string tag = boost::lexical_cast<std::string>(tags.at(i).get<double>());
			defense_matrix_vec.push_back(std::move(tag));
		}
		if(defense_matrix_vec.empty()){
			LOG_EMPERY_DUNGEON_WARNING("emptry defense matrix tag");
			return;
		}
		auto interval = boost::lexical_cast<std::uint64_t>(param_array.at(1).get<double>());
		auto utc_now =  Poseidon::get_utc_time();
		auto defense_matrix = boost::make_shared<DefenseMatrix>(virtual_shared_from_this<Dungeon>(),std::move(defense_matrix_vec),utc_now,interval);
		m_defense_matrixs.push_back(std::move(defense_matrix));
		pump_defense_matrix();
	} catch(std::exception &e){
		LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
	}
}

void Dungeon::on_triggers_dungeon_set_foot_annimation(const TriggerAction &action){
	PROFILE_ME;

	try{
		if(action.type != TriggerAction::A_SET_FOOT_ANNIMATION){
			return;
		}
		std::istringstream iss_param(action.params);
		auto param_array = Poseidon::JsonParser::parse_array(iss_param);
		if(param_array.size() != 6){
			LOG_EMPERY_DUNGEON_WARNING("dungeon set foot annimation  param error,size != 3",action.params);
			return;
		}
		std::vector<std::string> tags;
		auto picture_url =  param_array.at(0).get<std::string>();
		auto type   =  static_cast<int>(param_array.at(1).get<double>());
		auto x      =  static_cast<int>(param_array.at(3).get<double>());
		auto y      =  static_cast<int>(param_array.at(4).get<double>());
		auto layer  =  static_cast<int>(param_array.at(5).get<double>());
		auto &monster_array  =  param_array.at(2).get<Poseidon::JsonArray>();
		for(unsigned i = 0; i < monster_array.size(); ++i){
			auto monster_tag = boost::lexical_cast<std::string>(monster_array.at(i).get<double>());
			tags.push_back(std::move(monster_tag));
		}
		std::vector<boost::shared_ptr<DungeonObject>> valid_monsters;
		for(auto it = m_objects.begin(); it != m_objects.end(); ++it){
			auto &monster = it->second;
			std::string tag = monster->get_tag();
			auto itv = std::find(tags.begin(),tags.end(),tag);
			if(itv != tags.end()){
				valid_monsters.push_back(monster);
			}
		}
		if(valid_monsters.empty()){
			LOG_EMPERY_DUNGEON_WARNING("dungeon set foot annimation monster  empty");
			return;
		}

		const auto dungeon_client = get_dungeon_client();
		if(dungeon_client){
			try {
				Msg::DS_DungeonSetFootAnnimation msg;
				msg.dungeon_uuid    = get_dungeon_uuid().str();
				msg.picture_url     = picture_url;
				msg.type            = type;
				msg.x               = x;
				msg.y               = y;
				msg.layer           = layer;
				for(unsigned i = 0; i < valid_monsters.size(); ++i){
					auto &monsters = *msg.monsters.emplace(msg.monsters.end());
					monsters.monster_uuid = valid_monsters.at(i)->get_dungeon_object_uuid().str();
				}
				dungeon_client->send(msg);
			} catch(std::exception &e){
				LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
				dungeon_client->shutdown(e.what());
			}
		}
	} catch(std::exception &e){
		LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
	}
}

void Dungeon::pump_skill_damage(){
	PROFILE_ME;

	std::vector<boost::shared_ptr<SkillRecycleDamage>>   skill_damage;
	skill_damage.reserve(m_skill_damages.size()*5);
	const auto utc_now = Poseidon::get_utc_time();
	for(auto it = m_skill_damages.begin(); it != m_skill_damages.end(); ++it){
		auto &damage_vector = it->second;
		for(auto itd = damage_vector.begin(); itd != damage_vector.end(); ){
			auto &damage = *itd;
			if(damage->times == 0){
				LOG_EMPERY_DUNGEON_DEBUG("remove skill damage times == 0,skill_id = ",damage->skill_id);
				itd = damage_vector.erase(itd);
				continue;
			}else {
				if(utc_now < damage->next_damage_time){
					++itd;
					continue;
				}
				damage->times--;
				damage->next_damage_time = utc_now + damage->interval;
				skill_damage.push_back(damage);
				++itd;
			}
		}
	}
	if(m_fight_state == FIGHT_PAUSE){
		return;
	}
	for(auto it = skill_damage.begin(); it != skill_damage.end(); ++it){
		const auto &damage = *it;
		try {
			do_skill_damage(damage);
		} catch(std::exception &e){
			// log
		}
	}
}

void Dungeon::do_skill_damage(const boost::shared_ptr<SkillRecycleDamage>& skill_damage){
	PROFILE_ME;

	try{
		const auto dungeon_client = get_dungeon_client();
		if(!dungeon_client){
			return;
		}
		auto &coords = skill_damage->damage_range;
		std::vector<boost::shared_ptr<DungeonObject>> ret;
		auto skill_data  = Data::Skill::require(skill_damage->skill_id);
		auto attack_rate = skill_data->attack_rate;
		auto attack_fix  = skill_data->attack_fix;
		auto attack_type = skill_data->attack_type;
		double k = 0.03;
		auto total_attack = skill_damage->attack*attack_rate + attack_fix;
		if(skill_damage->skill_id == ID_SKILL_ENERGY_SPHERE){
			total_attack = attack_rate;
			LOG_EMPERY_DUNGEON_ERROR("SKILL ENERGY SPHERE,total_attack ",total_attack);
		}
		for(auto it = coords.begin(); it != coords.end(); ++it){
			auto &coord = *it;
			auto target_object = get_object(coord);
			if(target_object){
				if(target_object->get_owner_uuid() != skill_damage->owner_account_uuid){
					try {
							//伤害计算
							double relative_rate = Data::DungeonObjectRelative::get_relative(attack_type,target_object->get_arm_defence_type());
							double total_defense = target_object->get_total_defense();
							double damage = total_attack * 1 * relative_rate * (1 - k *total_defense/(1 + k*total_defense));
							double defense_matrix = get_dungeon_attribute(target_object->get_dungeon_object_uuid(),target_object->get_owner_uuid(),EmperyCenter::AttributeIds::ID_DEFENSE_MATRIX) / 1000.0;
							if(defense_matrix > 0.0){
								damage = damage*(1 - defense_matrix);
								LOG_EMPERY_DUNGEON_FATAL("defense matrix damage:",damage);
							}
							Msg::DS_DungeonObjectAttackAction msg;
							msg.dungeon_uuid           = get_dungeon_uuid().str();
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
	} catch(std::exception &e){
			LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
	}
}

void Dungeon::insert_skill_damage(const boost::shared_ptr<SkillRecycleDamage>& damage){
	PROFILE_ME;

	if(!damage){
		LOG_EMPERY_DUNGEON_WARNING("empty skill damage");
		return;
	}
	auto coord = damage->origin_coord;
	auto it = m_skill_damages.find(coord);
	if(it == m_skill_damages.end()){
		std::vector<boost::shared_ptr<SkillRecycleDamage>> damage_vec;
		damage_vec.push_back(damage);
		m_skill_damages.emplace(coord,std::move(damage_vec));
	}else{
		auto &damage_vec = it->second;
		damage_vec.push_back(damage);
	}
}

void Dungeon::check_skill_damage_move_pass(Coord coord,std::string params,bool isMonster){
	PROFILE_ME;

	if(isMonster){
		return;
	}
	auto utc_now = Poseidon::get_utc_time();
	auto it = m_skill_damages.find(coord);
	if(it != m_skill_damages.end()){
		auto &damage_vec = it->second;
		for(auto itd = damage_vec.begin(); itd != damage_vec.end();++itd){
			auto &skill_damage = *itd;
			if(skill_damage->skill_id == ID_SKILL_FIRE_BRAND){
				skill_damage->next_damage_time = utc_now;
			}
		}
	}
	pump_skill_damage();
}

void Dungeon::insert_skill_buff(DungeonObjectUuid dungeon_object_uuid,DungeonBuffTypeId buff_id,const boost::shared_ptr<DungeonBuff> dungeon_buff){
	PROFILE_ME;

	const auto dungeon_client = get_dungeon_client();
	if(!dungeon_client){
		LOG_EMPERY_DUNGEON_WARNING("empty dungeon client");
		return;
	}
	try{
		auto it = m_skill_buffs.find(std::make_pair(dungeon_object_uuid,buff_id));
		if(it == m_skill_buffs.end()){
			m_skill_buffs.emplace(std::make_pair(dungeon_object_uuid,buff_id),dungeon_buff);
		}else{
			it->second = dungeon_buff;
		}
		Msg::DS_DungeonObjectAddBuff msg;
		msg.dungeon_uuid        = get_dungeon_uuid().str();
		msg.dungeon_object_uuid = dungeon_object_uuid.str();
		msg.buff_type_id        = buff_id.get();
		dungeon_client->send(msg);
	} catch(std::exception &e){
		LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
		dungeon_client->shutdown(e.what());
	}
}

void Dungeon::remove_skill_buff(DungeonObjectUuid dungeon_object_uuid,DungeonBuffTypeId buff_id){
	const auto dungeon_client = get_dungeon_client();
	if(!dungeon_client){
		LOG_EMPERY_DUNGEON_WARNING("empty dungeon client");
		return;
	}
	try{
		auto it = m_skill_buffs.find(std::make_pair(dungeon_object_uuid,buff_id));
		if(it != m_skill_buffs.end()){
			m_skill_buffs.erase(it);
			Msg::DS_DungeonObjectClearBuff msg;
		    msg.dungeon_uuid        = get_dungeon_uuid().str();
			msg.dungeon_object_uuid = dungeon_object_uuid.str();
			msg.buff_type_id        = buff_id.get();
			LOG_EMPERY_DUNGEON_FATAL(msg);
			dungeon_client->send(msg);
		}
	} catch(std::exception &e){
		LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
		dungeon_client->shutdown(e.what());
	}
}

void Dungeon::pump_defense_matrix(){
	PROFILE_ME;

	std::vector<boost::shared_ptr<DefenseMatrix>> defense_matrixs;
	defense_matrixs.reserve(m_defense_matrixs.size());
	const auto utc_now = Poseidon::get_utc_time();
	for(auto it = m_defense_matrixs.begin(); it != m_defense_matrixs.end(); ++it){
		auto &defense_matrix = *it;
		if(defense_matrix->get_next_change_time() < utc_now || defense_matrix->is_ignore_die()){
			defense_matrixs.push_back(defense_matrix);
		}
	}

	for(auto it = defense_matrixs.begin(); it != defense_matrixs.end(); ++it){
		const auto &defense_matrix = *it;
		try {
			defense_matrix->change_defense_matrix(utc_now);
		} catch(std::exception &e){
			// log
		}
	}
}




}
