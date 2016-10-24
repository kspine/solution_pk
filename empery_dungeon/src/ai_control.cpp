#include "precompiled.hpp"
#include "ai_control.hpp"
#include "dungeon_object.hpp"
#include <poseidon/cbpp/status_codes.hpp>

namespace EmperyDungeon {


AiControl::AiControl(std::uint64_t unique_id,boost::weak_ptr<DungeonObject> parent)
:m_unique_id(unique_id),m_parent_object(parent)
{
}

AiControl::~AiControl(){
}

std::uint64_t  AiControl::get_ai_id(){
	PROFILE_ME;

	return m_unique_id;
}

std::uint64_t AiControl::move(std::pair<long, std::string> &result){
	PROFILE_ME;

	const auto parent_object = m_parent_object.lock();
	if(!parent_object){
		return UINT64_MAX;
	}
	return parent_object->move(result);
}

void AiControl::troops_attack(boost::shared_ptr<DungeonObject> target,bool passive){
	PROFILE_ME;
	const auto parent_object = m_parent_object.lock();
	if(!parent_object){
		return;
	}
	parent_object->troops_attack(target,passive);
	return ;
}

std::uint64_t AiControl::on_attack(boost::shared_ptr<DungeonObject> attacker,std::uint64_t demage){
	PROFILE_ME;
	const auto parent_object = m_parent_object.lock();
	if(!parent_object){
		return UINT64_MAX;
	}
	return parent_object->on_attack_common(attacker,demage);
}

std::uint64_t AiControl::on_action_attack(std::pair<long, std::string> &result, std::uint64_t now){
	PROFILE_ME;
	const auto parent_object = m_parent_object.lock();
	if(!parent_object){
		return UINT64_MAX;
	}
	return parent_object->attack(result,now);
}

std::uint64_t AiControl::on_action_monster_regress(std::pair<long, std::string> &result, std::uint64_t now){
	return UINT64_MAX;
}
std::uint64_t AiControl::on_action_monster_search_target(std::pair<long, std::string> &result, std::uint64_t now){
	return UINT64_MAX;
}
std::uint64_t AiControl::on_action_guard(std::pair<long, std::string> &result, std::uint64_t now){
	return UINT64_MAX;
}

std::uint64_t AiControl::on_action_patrol(std::pair<long, std::string> &result, std::uint64_t now){
	return UINT64_MAX;
}

std::uint64_t AiControl::on_lose_target(){
	PROFILE_ME;

	const auto parent_object = m_parent_object.lock();
	if(!parent_object){
		return UINT64_MAX;
	}
	return parent_object->lost_target_common();
}

AiControlMonsterAutoSearch::AiControlMonsterAutoSearch(std::uint64_t unique_id,boost::weak_ptr<DungeonObject> parent):AiControl(unique_id,parent){
}

AiControlMonsterAutoSearch::~AiControlMonsterAutoSearch(){}

std::uint64_t AiControlMonsterAutoSearch::on_lose_target(){
	PROFILE_ME;

	const auto parent_object = m_parent_object.lock();
	if(!parent_object){
		return UINT64_MAX;
	}
	return parent_object->lost_target_monster();
}

std::uint64_t AiControlMonsterAutoSearch::on_action_monster_regress(std::pair<long, std::string> &result, std::uint64_t now){
	PROFILE_ME;

	const auto parent_object = m_parent_object.lock();
	if(!parent_object){
		return UINT64_MAX;
	}
	return parent_object->on_monster_regress();
}
std::uint64_t AiControlMonsterAutoSearch::on_action_monster_search_target(std::pair<long, std::string> &result, std::uint64_t now){
	PROFILE_ME;

	const auto parent_object = m_parent_object.lock();
	if(!parent_object){
		return UINT64_MAX;
	}
	return parent_object->monster_search_attack_target(result);
}

std::uint64_t AiControlMonsterAutoSearch::on_action_guard(std::pair<long, std::string> &result, std::uint64_t now){
	PROFILE_ME;

	const auto parent_object = m_parent_object.lock();
	if(!parent_object){
		return UINT64_MAX;
	}
	return parent_object->on_monster_guard();
}

AiControlMonsterPatrol::AiControlMonsterPatrol(std::uint64_t unique_id,boost::weak_ptr<DungeonObject> parent):AiControl(unique_id,parent){
}

AiControlMonsterPatrol::~AiControlMonsterPatrol(){}

std::uint64_t AiControlMonsterPatrol::move(std::pair<long, std::string> &result){
	PROFILE_ME;

	const auto parent_object = m_parent_object.lock();
	if(!parent_object){
		return UINT64_MAX;
	}
	if(DungeonObject::ACT_MONSTER_PATROL == parent_object->get_action()){
		std::pair<long, std::string> search_result;
		std::uint64_t search_delay = parent_object->monster_search_attack_target(search_result);
		if(search_result.first == Poseidon::Cbpp::ST_OK){
			LOG_EMPERY_DUNGEON_DEBUG("patrol search target sucess");
			return search_delay;
		}
	}
	return AiControl::move(result);
}

std::uint64_t AiControlMonsterPatrol::on_lose_target(){
	PROFILE_ME;

	const auto parent_object = m_parent_object.lock();
	if(!parent_object){
		return UINT64_MAX;
	}
	return parent_object->on_monster_guard();
}

std::uint64_t AiControlMonsterPatrol::on_action_monster_search_target(std::pair<long, std::string> &result, std::uint64_t now){
	PROFILE_ME;

	const auto parent_object = m_parent_object.lock();
	if(!parent_object){
		return UINT64_MAX;
	}
	return parent_object->monster_search_attack_target(result);
}

std::uint64_t AiControlMonsterPatrol::on_action_guard(std::pair<long, std::string> &result, std::uint64_t now){
	PROFILE_ME;

	const auto parent_object = m_parent_object.lock();
	if(!parent_object){
		return UINT64_MAX;
	}
	return parent_object->on_monster_guard();
}

std::uint64_t AiControlMonsterPatrol::on_action_patrol(std::pair<long, std::string> &result, std::uint64_t now){
	PROFILE_ME;

	const auto parent_object = m_parent_object.lock();
	if(!parent_object){
		return UINT64_MAX;
	}
	return parent_object->on_monster_patrol();
}

AiControlMonsterObject::AiControlMonsterObject(std::uint64_t unique_id,boost::weak_ptr<DungeonObject> parent):AiControl(unique_id,parent){
}

AiControlMonsterObject::~AiControlMonsterObject(){}

std::uint64_t AiControlMonsterObject::move(std::pair<long, std::string> &result){
	PROFILE_ME;

	return UINT64_MAX;
}

std::uint64_t AiControlMonsterObject::on_attack(boost::shared_ptr<DungeonObject> attacker,std::uint64_t demage){
	return UINT64_MAX;
}

void          AiControlMonsterObject::troops_attack(boost::shared_ptr<DungeonObject> target,bool passive){
	return ;
}



}