#ifndef EMPERY_CLUSTER_AI_CONTROL_HPP_
#define EMPERY_CLUSTER_AI_CONTROL_HPP_

namespace EmperyCluster {

class MapObject;
class ResourceCrate;
class MapCell;

class AiControl{
public:
	AiControl(std::uint64_t unique_id,boost::weak_ptr<MapObject> parent);
	virtual ~AiControl();
public:
	virtual std::uint64_t move(std::pair<long, std::string> &result);
	virtual std::uint64_t attack(std::pair<long, std::string> &result, std::uint64_t now);
	virtual void          troops_attack(boost::shared_ptr<MapObject> target,bool passive = false);
	virtual std::uint64_t on_attack(boost::shared_ptr<MapObject> attacker,std::uint64_t demage);
	virtual std::uint64_t harvest_resource_crate(std::pair<long, std::string> &result, std::uint64_t now,bool forced_attack = false);
	virtual std::uint64_t attack_territory(std::pair<long, std::string> &result, std::uint64_t now,bool forced_attack = false);
	virtual std::uint64_t patrol();
	virtual std::uint64_t on_lose_target();
public:
	std::uint64_t          get_ai_id();
public:
	std::uint64_t              m_unique_id;
	boost::weak_ptr<MapObject> m_parent_object;
};

class AiControlDefenseBuilding : public AiControl {
public:
	AiControlDefenseBuilding(std::uint64_t unique_id,boost::weak_ptr<MapObject> parent);
	~AiControlDefenseBuilding();
public:
	std::uint64_t move(std::pair<long, std::string> &result) override;
};

class AiControlDefenseBuildingNoAttack : public AiControlDefenseBuilding{
public:
	AiControlDefenseBuildingNoAttack(std::uint64_t unique_id,boost::weak_ptr<MapObject> parent);
	~AiControlDefenseBuildingNoAttack();
public:
	std::uint64_t attack(std::pair<long, std::string> &result, std::uint64_t now) override;
};

class AiControlMonsterCommon : public AiControl {
public:
	AiControlMonsterCommon(std::uint64_t unique_id,boost::weak_ptr<MapObject> parent);
	~AiControlMonsterCommon();
public:
	std::uint64_t move(std::pair<long, std::string> &result) override;
	std::uint64_t on_lose_target() override;
};

class AiControlMonsterGoblin : public AiControl{
public:
	AiControlMonsterGoblin(std::uint64_t unique_id,boost::weak_ptr<MapObject> parent);
	~AiControlMonsterGoblin();
public:
	std::uint64_t on_attack(boost::shared_ptr<MapObject> attacker,std::uint64_t demage) override;
};

}

#endif
