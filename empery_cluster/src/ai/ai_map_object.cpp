#include "../precompiled.hpp"
#include "ai_map_object.hpp"
#include "../map_object.hpp"

namespace EmperyCluster {
AI_MapObject::AI_MapObject(boost::weak_ptr<MapObject> owner){
	m_mapObject = std::move(owner);
}

AI_MapObject::~AI_MapObject(){
}

std::uint64_t AI_MapObject::AI_Move(std::pair<long, std::string> &result){
	auto mapObject = m_mapObject.lock();
	return mapObject->move(result);
}

std::uint64_t AI_MapObject::AI_Combat(std::pair<long, std::string> &result, std::uint64_t now){
	return UINT64_MAX;
}

std::uint64_t AI_MapObject::AI_ON_Combat(boost::shared_ptr<MapObject> attacker,std::uint64_t demage){
	return UINT64_MAX;
}

std::uint64_t  AI_MapObject::AI_ON_Die(boost::shared_ptr<MapObject> attacker){
	return UINT64_MAX;
}




}
