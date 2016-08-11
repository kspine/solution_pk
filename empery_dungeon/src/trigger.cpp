#include "precompiled.hpp"
#include "trigger.hpp"
#include <poseidon/cbpp/status_codes.hpp>

namespace EmperyDungeon {
	Trigger::Trigger(DungeonUuid dungeon_uuid_,std::string name_,std::uint64_t delay_,TriggerCondition condition_,std::deque<TriggerAction> actions_,bool activited_,std::uint64_t activated_time_)
	:dungeon_uuid(dungeon_uuid_),name(name_),delay(delay_),condition(condition_),actions(std::move(actions_)),activated(activited_),activated_time(activated_time_){
	}
	Trigger::~Trigger(){
	}

	void Trigger::pump_action(){
	}
}