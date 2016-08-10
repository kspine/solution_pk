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


Dungeon::Dungeon(DungeonUuid dungeon_uuid, DungeonTypeId dungeon_type_id,const boost::shared_ptr<DungeonClient> &dungeon,AccountUuid founder_uuid)
	: m_dungeon_uuid(dungeon_uuid), m_dungeon_type_id(dungeon_type_id)
	, m_dungeon_client(dungeon)
	, m_founder_uuid(founder_uuid)
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
	for(auto it = m_triggers.begin(); it != m_triggers.end(); ++it){
		const auto &trigger = it->second;
		if(!trigger->activated){
			continue;
		}
		trigger->activated = false;
		triggers_activated.emplace_back(trigger);
	}
	for(auto it = triggers_activated.begin(); it != triggers_activated.end(); ++it){
		const auto &trigger = *it;
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

void Dungeon::set_founder_uuid(AccountUuid founder_uuid){
	PROFILE_ME;

	m_founder_uuid = founder_uuid;

	DungeonMap::update(virtual_shared_from_this<Dungeon>(), false);
}

void Dungeon::set_dungeon_duration(std::uint64_t duration){
	PROFILE_ME;

	/*
	const auto timer_proc = [this](const boost::weak_ptr<Dungeon> &weak, std::uint64_t now){
		PROFILE_ME;

		const auto shared = weak.lock();
		if(!shared){
			return;
		}
		m_dungeon_timer.reset();
		const auto dungeon_client = shared->get_dungeon_client();
		if(dungeon_client){
			try {
				Msg::DS_DungeonPlayerWins msg;
				msg.dungeon_uuid        = shared->get_dungeon_uuid().str();
				msg.account_uuid        = shared->get_founder_uuid().str();
				dungeon_client->send(msg);
			} catch(std::exception &e){
				LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
				dungeon_client->shutdown(e.what());
			}
		}
	};
	if(!m_dungeon_timer){
		const auto now = Poseidon::get_fast_mono_clock();
		auto timer = Poseidon::TimerDaemon::register_absolute_timer(now + duration, 0,
			std::bind(timer_proc, virtual_weak_from_this<Dungeon>(), std::placeholders::_2));
		LOG_EMPERY_DUNGEON_DEBUG("Created dungeon expired timer: dungeon_uuid = ", get_dungeon_uuid());
		m_dungeon_timer = std::move(timer);
	}
	*/
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

void Dungeon::reove_dungeon_object_no_synchronize(DungeonObjectUuid dungeon_object_uuid){
	PROFILE_ME;

	const auto it = m_objects.find(dungeon_object_uuid);
	if(it == m_objects.end()){
		LOG_EMPERY_DUNGEON_TRACE("Dungeon object not found: dungeon_object_uuid = ", dungeon_object_uuid, ", dungeon_uuid = ", get_dungeon_uuid());
		return;
	}else{
		m_objects.erase(it);
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
	for(auto it = ret.begin(); it != ret.end(); ++it){
		auto trigger_name = (*it)->trigger_name;
		auto type      = static_cast<TriggerCondition::Type>((*it)->type);
		auto effect = (*it)->effect;
		auto effect_param = (*it)->effect_params;
		std::deque<TriggerAction> actions;
		auto activited = (*it)->activated;
		parse_triggers_action(actions,std::move(effect),std::move(effect_param));
		auto triggers = boost::make_shared<Trigger>(get_dungeon_uuid(),trigger_name,type,std::move(actions),activited,0);
		const auto result = m_triggers.emplace(trigger_name, std::move(triggers));
		if(!result.second){
			LOG_EMPERY_DUNGEON_WARNING("Dungeon trigger already exists: dungeon_type_id = ", get_dungeon_type_id(), ", trigger_name = ", trigger_name);
			DEBUG_THROW(Exception, sslit("Dungeon trigger already exists"));
		}
	}
}

void Dungeon::check_triggers(const TriggerCondition &condition){
	PROFILE_ME;

	switch(condition.type){
		{
#define ON_TRIGGER(trigger_type)	\
		}	\
		break;	\
	case (trigger_type): {
//=============================================================================
	ON_TRIGGER(TriggerCondition::C_ENTER_DUNGEON){
		//进入副本触发
		check_triggers_enter_dungeon(condition);
	}
	ON_TRIGGER(TriggerCondition::C_DUNGEON_OBJECT_PASS){
		//副本对象通过触发
	}
	ON_TRIGGER(TriggerCondition::C_DUNGEON_OBJECT_HP){
		//副本对象HP触发触发
	}
	ON_TRIGGER(TriggerCondition::C_DELAY){
		//进入副本延迟触发
	}
//=============================================================================
#undef ON_TRIGGER
		}
		break;
	default:
		LOG_EMPERY_DUNGEON_WARNING("Unknown trigger type = ", static_cast<unsigned>(condition.type));
		break;
	}
}

void Dungeon::parse_triggers_action(std::deque<TriggerAction> &actions,std::string effect,std::string effect_param){
	PROFILE_ME;

	std::istringstream iss_effect(effect);
	auto effect_array = Poseidon::JsonParser::parse_array(iss_effect);
	for(unsigned i  = 0; i < effect_array.size(); ++i){
		TriggerAction triggerAction;
		auto &effects           = effect_array.at(i).get<Poseidon::JsonArray>();
		triggerAction.delay     = static_cast<std::uint64_t>(effects.at(0).get<double>());
		triggerAction.type      = static_cast<TriggerAction::Type>(effects.at(1).get<double>());
		if(!effect_param.empty()){
			std::istringstream iss_effect_param(effect_param);
			auto effect_param_array = Poseidon::JsonParser::parse_array(iss_effect_param);
			if(i <= (effect_param_array.size() -1) ){
				triggerAction.params = effect_param_array.at(i).dump();
			}
		}
		LOG_EMPERY_DUNGEON_DEBUG("parse triggers_action ,raw_effect = ",effect," raw_effect_param = ",effect_param," type = ", triggerAction.type, " delay = ",triggerAction.delay, " params = ", triggerAction.params);
		actions.push_back(std::move(triggerAction));
	}
}

void Dungeon::check_triggers_enter_dungeon(const TriggerCondition &condition){
	const auto utc_now = Poseidon::get_utc_time();
	for(auto it = m_triggers.begin(); it != m_triggers.end(); ++it){
		const auto &trigger = it->second;
		if(!trigger->activated && trigger->type == TriggerCondition::C_ENTER_DUNGEON){
			trigger->activated = true;
			trigger->activated_time = utc_now;
		}
	}
}

void Dungeon::on_triggers_action(const TriggerAction &action){
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
	}
	ON_TRIGGER_ACTION(TriggerAction::A_BUFF){
		//TODO
	}
	ON_TRIGGER_ACTION(TriggerAction::A_SET_TRIGGER){
		//TODO
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
				Msg::DS_DungeonCreateMonster msgCreateMonster;
				msgCreateMonster.dungeon_uuid       = get_dungeon_uuid().str();
				msgCreateMonster.map_object_type_id = static_cast<std::uint64_t>(elem.at(0).get<double>());
				msgCreateMonster.x                  = static_cast<int>(elem.at(1).get<double>());
				msgCreateMonster.y                  = static_cast<int>(elem.at(2).get<double>());
				msgCreateMonster.tag               =  boost::lexical_cast<std::string>(elem.at(3).get<double>());
				dungeon_client->send(msgCreateMonster);
				LOG_EMPERY_DUNGEON_DEBUG("msg create monster:",msgCreateMonster);
			} catch(std::exception &e){
				LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
				dungeon_client->shutdown(e.what());
			}
		}

	}
}

}
