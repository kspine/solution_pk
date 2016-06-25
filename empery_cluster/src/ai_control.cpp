#include "precompiled.hpp"
#include "ai_control.hpp"
#include "map_object.hpp"

namespace EmperyCluster {


AiControl::AiControl(std::uint64_t unique_id,boost::weak_ptr<MapObject> parent)
:m_unique_id(unique_id),m_parent_object(parent)
{
}

AiControl::~AiControl(){
	
}

std::uint64_t AiControl::move(std::pair<long, std::string> &result){
	PROFILE_ME;
	
	const auto parent_object = m_parent_object.lock();
	if(!parent_object){
		return UINT64_MAX;
	}
	return parent_object->move(result);
}

std::uint64_t AiControl::attack(std::pair<long, std::string> &result, std::uint64_t now){
	PROFILE_ME;
	const auto parent_object = m_parent_object.lock();
	if(!parent_object){
		return UINT64_MAX;
	}
	return parent_object->attack(result,now);
}

void          AiControl::troops_attack(boost::shared_ptr<MapObject> target,bool passive){
	PROFILE_ME;
	const auto parent_object = m_parent_object.lock();
	if(!parent_object){
		return;
	}
	parent_object->troops_attack(target,passive);
	return ;
}

std::uint64_t AiControl::on_attack(boost::shared_ptr<MapObject> attacker,std::uint64_t demage){
	PROFILE_ME;
	const auto parent_object = m_parent_object.lock();
	if(!parent_object){
		return UINT64_MAX;
	}
	return parent_object->on_attack(attacker,demage);
}

std::uint64_t AiControl::harvest_resource_crate(std::pair<long, std::string> &result, std::uint64_t now,bool force_attack){
	PROFILE_ME;
	const auto parent_object = m_parent_object.lock();
	if(!parent_object){
		return UINT64_MAX;
	}
	return parent_object->harvest_resource_crate(result,now,force_attack);
}

std::uint64_t AiControl::attack_territory(std::pair<long, std::string> &result, std::uint64_t now,bool forced_attack){
	PROFILE_ME;
	const auto parent_object = m_parent_object.lock();
	if(!parent_object){
		return UINT64_MAX;
	}
	return parent_object->attack_territory(result,now,forced_attack);
}

std::uint64_t AiControl::patrol(){
	return 0;
}
std::uint64_t AiControl::on_lose_target(){
	return 0;
}

std::uint64_t AiControlMonsterCommon::on_lose_target(){
	return 0;
}

std::uint64_t AiControlMonsterGoblin::on_attack(boost::shared_ptr<MapObject> attacker,std::uint64_t demage){
	return 0;
}

std::uint64_t AiControlMonsterActive::on_attack(boost::shared_ptr<MapObject> attacker,std::uint64_t demage){
	return 0;
}

std::uint64_t AiControlMonsterActive::patrol(){
	return 0;
}

std::uint64_t AiControlMonsterPatrol::on_attack(boost::shared_ptr<MapObject> attacker,std::uint64_t demage){
	return 0;
}

std::uint64_t AiControlMonsterPatrol::patrol(){
	return 0;
}

}