#include "precompiled.hpp"
#include "trigger.hpp"
#include <poseidon/cbpp/status_codes.hpp>

namespace EmperyDungeon {
	Trigger::Trigger(DungeonUuid dungeon_uuid_,std::string name_,std::uint64_t delay_,TriggerCondition condition_,std::deque<TriggerAction> actions_,bool activited_,std::uint64_t activated_time_)
	:dungeon_uuid(dungeon_uuid_),name(name_),delay(delay_),condition(condition_),actions(std::move(actions_)),activated(activited_),activated_time(activated_time_){
	}
	Trigger::~Trigger(){
	}
	
	TriggerDamage::TriggerDamage(bool is_target_monster_,std::uint64_t loop_,std::uint64_t damage_,std::uint64_t delay_, std::uint64_t next_damage_time_,std::vector<Coord> damage_range_)
	:is_target_monster(is_target_monster_),loop(loop_),damage(damage_),delay(delay_),next_damage_time(next_damage_time),damage_range(std::move(damage_range_)){
	}

	TriggerDamage::~TriggerDamage(){
	}
}