#ifndef EMPERY_CLUSTER_AI_MAP_OBJECT_HPP_
#define EMPERY_CLUSTER_AI_MAP_OBJECT_HPP_
namespace EmperyCluster {
class MapObject;
class AI_MapObject{
public:
	AI_MapObject(boost::weak_ptr<MapObject> owner);
	virtual ~AI_MapObject();
	
public:
	virtual std::uint64_t  AI_Move(std::pair<long, std::string> &result);
	virtual std::uint64_t  AI_Combat(std::pair<long, std::string> &result, std::uint64_t now);
	virtual std::uint64_t  AI_ON_Combat(boost::shared_ptr<MapObject> attacker,std::uint64_t demage);
	virtual std::uint64_t  AI_ON_Die(boost::shared_ptr<MapObject> attacker);
protected:
	boost::weak_ptr<MapObject> m_mapObject;
};

}

#endif
  