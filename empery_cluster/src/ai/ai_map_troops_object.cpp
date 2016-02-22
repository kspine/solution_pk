#include "../precompiled.hpp"
#include "ai_map_troops_object.hpp"
#include "../map_object.hpp"
namespace EmperyCluster {
AI_MapTroopsObject::AI_MapTroopsObject(boost::weak_ptr<MapObject> owner)
:AI_MapObject(std::move(owner)){}

AI_MapTroopsObject::~AI_MapTroopsObject(){
}


std::uint64_t AI_MapTroopsObject::AI_Move(std::pair<long, std::string> &result){
	auto mapObject = m_mapObject.lock();
	return mapObject->move(result);
}

std::uint64_t AI_MapTroopsObject::AI_Combat(std::pair<long, std::string> &result, std::uint64_t now){
	auto mapObject = m_mapObject.lock();
	return mapObject->combat(result,now);
}

std::uint64_t AI_MapTroopsObject::AI_ON_Combat(boost::shared_ptr<MapObject> attacker,std::uint64_t demage){
	auto mapObject = m_mapObject.lock();
	return mapObject->on_combat(attacker,demage);
}

std::uint64_t  AI_MapTroopsObject::AI_ON_Die(boost::shared_ptr<MapObject> attacker){
	auto mapObject = m_mapObject.lock();
	return mapObject->on_die(attacker);
}


}
