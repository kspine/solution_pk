#ifndef EMPERY_CLUSTER_AI_MAP_TROOPS_OBJECT_HPP_
#define EMPERY_CLUSTER_AI_MAP_TROOPS_OBJECT_HPP_
#include "ai_map_object.hpp"

namespace EmperyCluster {
class MapObject;
class AI_MapTroopsObject : public AI_MapObject{
public:
	AI_MapTroopsObject(boost::weak_ptr<MapObject> owner);
	~AI_MapTroopsObject() override;
public:
	std::uint64_t  AI_Move(std::pair<long, std::string> &result) override;
	std::uint64_t  AI_Combat(std::pair<long, std::string> &result, std::uint64_t now) override;
	std::uint64_t  AI_ON_Combat(boost::shared_ptr<MapObject> attacker,std::uint64_t demage) override;
	std::uint64_t  AI_ON_Die(boost::shared_ptr<MapObject> attacker) override;
};

}

#endif
  