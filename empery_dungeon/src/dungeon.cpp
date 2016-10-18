#include "precompiled.hpp"
#include "dungeon.hpp"
#include "singletons/dungeon_map.hpp"
#include "src/dungeon_object.hpp"
#include "src/dungeon_client.hpp"
#include "src/data/dungeon.hpp"
#include "src/data/trigger.hpp"
#include <poseidon/singletons/timer_daemon.hpp>
#include "../../empery_center/src/msg/ds_dungeon.hpp"
#include "trigger.hpp"
#include <poseidon/json.hpp>
#include "mmain.hpp"
#include <poseidon/cbpp/status_codes.hpp>
#include "dungeon_utilities.hpp"
#include "src/data/global.hpp"
#include "src/data/dungeon_object.hpp"
#include "src/dungeon_trap.hpp"

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


Dungeon::Dungeon(DungeonUuid dungeon_uuid, DungeonTypeId dungeon_type_id,const boost::shared_ptr<DungeonClient> &dungeon,AccountUuid founder_uuid,std::uint64_t create_time)
	: m_dungeon_uuid(dungeon_uuid), m_dungeon_type_id(dungeon_type_id)
	, m_dungeon_client(dungeon)
	, m_founder_uuid(founder_uuid)
	, m_create_dungeon_time(create_time)
	, m_dungeon_state(S_INIT)
	, m_monster_removed_count(0)
{
}

Dungeon::~Dungeon(){
}

void Dungeon::pump_status(){
	PROFILE_ME;

	//
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

void Dungeon::update_damage_solider(DungeonObjectTypeId dungeon_object_type_id,std::uint64_t damage_solider){
	auto it = m_damage_solider.find(dungeon_object_type_id);
	if(it == m_damage_solider.end()){
		m_damage_solider.emplace(dungeon_object_type_id, damage_solider);
		return;
	}else{
		m_damage_solider.at(it->first) = it->second + damage_solider;
	}
}

std::uint64_t Dungeon::get_total_damage_solider(){
	std::uint64_t total_damage_solider = 0;
	for(auto it = m_damage_solider.begin(); it != m_damage_solider.end(); ++it){
		total_damage_solider += it->second;
	}
	return total_damage_solider;
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
	if(!pos_occuried && pos_passable){
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
	get_surrounding_coords(surrounding,src_coord, 3);
	for(auto it = surrounding.begin(); it != surrounding.end();++it){
		src_valid =  check_valid_coord_for_birth(*it);
		if(src_valid){
			dest_coord = *it;
			return true;
		}
	}
	return false;
}

boost::shared_ptr<DungeonTrap> Dungeon::get_trap(const Coord coord) const{
	PROFILE_ME;

	const auto it = m_traps.find(coord);
	if(it == m_traps.end()){
		LOG_EMPERY_DUNGEON_DEBUG("Dungeon trap not found: coord = ", coord);
		return { };
	}
	return it->second;
}

void Dungeon::insert_trap(const boost::shared_ptr<DungeonTrap> &dungeon_trap){
	PROFILE_ME;

	const auto dungeon_uuid = get_dungeon_uuid();
	if(dungeon_trap->get_dungeon_uuid() != dungeon_uuid){
		LOG_EMPERY_DUNGEON_WARNING("This dungeon trap does not belong to this dungeon!");
		DEBUG_THROW(Exception, sslit("This dungeon trap does not belong to this dungeon"));
	}

	const auto coord = dungeon_trap->get_coord();

	LOG_EMPERY_DUNGEON_DEBUG("Inserting dungeon trap: coord = ", coord, ", dungeon_uuid = ", dungeon_uuid);
	const auto result = m_traps.emplace(coord, dungeon_trap);
	if(!result.second){
		LOG_EMPERY_DUNGEON_WARNING("Dungeon trap already exists: coord = ", coord, ", dungeon_uuid = ", dungeon_uuid);
		DEBUG_THROW(Exception, sslit("Dungeon trap already exists"));
	}
}

void Dungeon::update_trap(const boost::shared_ptr<DungeonTrap> &dungeon_trap, bool throws_if_not_exists){
	PROFILE_ME;

	const auto dungeon_uuid = get_dungeon_uuid();
	if(dungeon_trap->get_dungeon_uuid() != dungeon_uuid){
		LOG_EMPERY_DUNGEON_WARNING("This dungeon trap does not belong to this dungeon!");
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("This dungeon trap does not belong to this dungeon"));
		}
		return;
	}

	const auto coord = dungeon_trap->get_coord();

	const auto it = m_traps.find(coord);
	if(it == m_traps.end()){
		LOG_EMPERY_DUNGEON_TRACE("Dungeon trap not found: coord = ", coord, ", dungeon_uuid = ", dungeon_uuid);
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Dungeon trap not found"));
		}
		return;
	}
	m_traps.at(it->first) = dungeon_trap;
	/*
	auto dungeon_client = get_dungeon_client();
	if(dungeon_client){
		notify_dungeon_object_updated(dungeon_object,dungeon_client);
	}
	*/
}

void Dungeon::replace_dungeon_trap_no_synchronize(const boost::shared_ptr<DungeonTrap> &dungeon_trap){
	PROFILE_ME;

	const auto dungeon_uuid = get_dungeon_uuid();
	if(dungeon_trap->get_dungeon_uuid() != dungeon_uuid){
		LOG_EMPERY_DUNGEON_WARNING("This dungeon trap does not belong to this dungeon!");
		return;
	}
	const auto coord = dungeon_trap->get_coord();
	const auto it = m_traps.find(coord);
	if(it == m_traps.end()){
		m_traps.emplace(coord, dungeon_trap);
		return;
	}else{
		m_traps.at(it->first) = dungeon_trap;
	}
}

void Dungeon::remove_dungeon_trap_no_synchronize(const Coord coord){
	PROFILE_ME;

	const auto it = m_traps.find(coord);
	if(it == m_traps.end()){
		LOG_EMPERY_DUNGEON_TRACE("Dungeon trap not found: coord = ", coord, ", dungeon_uuid = ", get_dungeon_uuid());
		return;
	}else{
		m_traps.erase(it);
	}
}

void Dungeon::check_trap_move_pass(Coord coord){
	PROFILE_ME;

	const auto trap = get_trap(coord);
	if(!trap)
		return;
	LOG_EMPERY_DUNGEON_FATAL("find a trap in ",coord);
	do_trap_damage(trap);
	//TODO删除陷阱
	const auto dungeon_client = get_dungeon_client();
	if(!dungeon_client){
		LOG_EMPERY_DUNGEON_ERROR("DO TRAP DAMAGE ERROR,CAN'T GET DUNGEON CLIENT");
		return;
	}
	try {
		Msg::DS_DungeonDeleteTrap msg;
		msg.dungeon_uuid = get_dungeon_uuid().str();
		msg.x = coord.x();
		msg.y = coord.y();
		auto sresult = dungeon_client->send_and_wait(msg);
		if(sresult.first != Poseidon::Cbpp::ST_OK){
			LOG_EMPERY_DUNGEON_DEBUG("trap damge fail", sresult.first, ", msg = ", sresult.second);
		}
	} catch(std::exception &e){
		LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
		dungeon_client->shutdown(e.what());
	}
}

void Dungeon::do_trap_damage(const boost::shared_ptr<DungeonTrap>& dungeon_trap){
	if(!dungeon_trap)
		return;
	const auto dungeon_client = get_dungeon_client();
	if(!dungeon_client){
		LOG_EMPERY_DUNGEON_ERROR("DO TRAP DAMAGE ERROR,CAN'T GET DUNGEON CLIENT");
		return;
	}
	double k = 0.35;
	const auto trap_data = Data::DungeonTrap::require(dungeon_trap->get_dungeon_trap_type_id());
	std::vector<Coord> surrounding;
	for(unsigned i = 0; i <= trap_data->attack_range;++i){
		get_surrounding_coords(surrounding,dungeon_trap->get_coord(),i);
	}
	std::vector<boost::shared_ptr<DungeonObject>> ret;
	get_objects_all(ret);
	LOG_EMPERY_DUNGEON_FATAL("surrounding coord size:",surrounding.size()," attack_range:",trap_data->attack_range," dungeon_object_size:",ret.size());
	for(auto it = surrounding.begin(); it != surrounding.end();++it){
		auto &coord = *it;
		for(auto itt = ret.begin(); itt != ret.end(); ++itt){
			const auto target_object = *itt;
			LOG_EMPERY_DUNGEON_FATAL("surrounding coord:",coord ," target_coord:",target_object->get_coord());
			if(coord == target_object->get_coord()){
				try {
					//伤害计算
					double relative_rate = Data::DungeonObjectRelative::get_relative(trap_data->attack_type,target_object->get_arm_defence_type());
					double total_defense = target_object->get_total_defense();
					double damage = trap_data->attack * 1 * relative_rate * (1 - k *total_defense/(1 + k*total_defense));
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
					LOG_EMPERY_DUNGEON_FATAL(msg);
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
		TriggerCondition trigger_conditon;
		trigger_conditon.type      = static_cast<TriggerCondition::Type>((*it)->type);
		trigger_conditon.params  = (*it)->condition;
		auto effect = (*it)->effect;
		auto effect_param = (*it)->effect_params;
		std::deque<TriggerAction> actions;
		auto activated = (*it)->activated;
		parse_triggers_action(actions,std::move(effect),std::move(effect_param));
		std::uint64_t activated_time = 0;
		if(activated){
			activated_time = utc_now;
		}
		auto triggers = boost::make_shared<Trigger>(get_dungeon_uuid(),trigger_id,delay,std::move(trigger_conditon),std::move(actions),activated,activated_time);
		const auto result = m_triggers.emplace(trigger_id, std::move(triggers));
		if(!result.second){
			LOG_EMPERY_DUNGEON_WARNING("Dungeon trigger already exists: dungeon_type_id = ", get_dungeon_type_id(), ", trigger_id = ", trigger_id);
			DEBUG_THROW(Exception, sslit("Dungeon trigger already exists"));
		}
	}

	const auto timer_proc = [this](const boost::weak_ptr<Dungeon> &weak, std::uint64_t now){
		PROFILE_ME;

		const auto shared = weak.lock();
		if(!shared){
			return;
		}
		shared->pump_triggers();
		shared->pump_triggers_damage();
	};
	if(!m_trigger_timer){
		//const auto now = Poseidon::get_fast_mono_clock();
		const auto trigger_interval = get_config<std::uint64_t>("trigger_check_interval", 1000);
		auto timer = Poseidon::TimerDaemon::register_timer(0, trigger_interval,
			std::bind(timer_proc,virtual_weak_from_this<Dungeon>(), std::placeholders::_2));
		m_trigger_timer = std::move(timer);
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
		if(!trigger->activated && trigger->condition.type == TriggerCondition::C_ENTER_DUNGEON){
			trigger->activated = true;
			trigger->activated_time = utc_now;
			forcast_triggers(trigger,utc_now);
		}
	}
	pump_triggers();
}

void Dungeon::check_triggers_move_pass(Coord coord,bool isMonster){
	PROFILE_ME;

	try{
		const auto utc_now = Poseidon::get_utc_time();
		for(auto it = m_triggers.begin(); it != m_triggers.end(); ++it){
			const auto &trigger = it->second;
			if(!trigger->activated && trigger->condition.type == TriggerCondition::C_DUNGEON_OBJECT_PASS){
				try{
					std::istringstream iss_params(trigger->condition.params);
					auto params_array = Poseidon::JsonParser::parse_array(iss_params);
					bool parmas_monster = params_array.at(0).get<double>();
					if(isMonster != parmas_monster){
						continue;
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
			if(!trigger->activated && trigger->condition.type == TriggerCondition::C_DUNGEON_OBJECT_HP){
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
			if(!trigger->activated && trigger->condition.type == TriggerCondition::C_DUNGEON_FINISH){
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
			if(!trigger->activated && trigger->condition.type == TriggerCondition::C_ALL_DIE){
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
	ON_TRIGGER_ACTION(TriggerAction::A_TRAP){
		//TODO
		on_triggers_create_dungeon_trap(action);
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
			if(!trigger->activated&&open){
				trigger->activated_time = utc_now;
			}
			trigger->activated = open;
			if(open){
				forcast_triggers(trigger,utc_now);
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

void Dungeon::on_triggers_create_dungeon_trap(const TriggerAction &action){
	PROFILE_ME;

	try{
		if(action.type != TriggerAction::A_TRAP){
			return;
		}
		std::istringstream iss_effect_param(action.params);
		auto effect_param_array = Poseidon::JsonParser::parse_array(iss_effect_param);
		const auto dungeon_client = get_dungeon_client();
		for(auto it = effect_param_array.begin(); it != effect_param_array.end(); ++it){
			auto &elem = it->get<Poseidon::JsonArray>();
			if(elem.size() != 2){
				LOG_EMPERY_DUNGEON_WARNING("create dungeon trap params error = ");
				continue;
			}
			if(dungeon_client){
				try {
					auto &birth                         = elem.at(1).get<Poseidon::JsonArray>();
					auto birth_x                        = static_cast<int>(birth.at(0).get<double>());
					auto birth_y                        = static_cast<int>(birth.at(1).get<double>());
					auto trap_type_id                   = static_cast<std::uint64_t>(elem.at(0).get<double>());
					Msg::DS_DungeonCreateTrap msgCreateTrap;
					msgCreateTrap.dungeon_uuid       = get_dungeon_uuid().str();
					msgCreateTrap.x                  = birth_x;
					msgCreateTrap.y                  = birth_y;
					msgCreateTrap.trap_type_id       = trap_type_id;
					dungeon_client->send(msgCreateTrap);
					LOG_EMPERY_DUNGEON_DEBUG("msg create trap:",msgCreateTrap);
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

}
