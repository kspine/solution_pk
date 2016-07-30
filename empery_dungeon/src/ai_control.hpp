#ifndef EMPERY_DUNGEON_AI_CONTROL_HPP_
#define EMPERY_DUNGEON_AI_CONTROL_HPP_

namespace EmperyDungeon {

class DungeonObject;

class AiControl{
public:
	AiControl(std::uint64_t unique_id,boost::weak_ptr<DungeonObject> parent);
	virtual ~AiControl();
public:
	virtual std::uint64_t move(std::pair<long, std::string> &result);
	virtual std::uint64_t attack(std::pair<long, std::string> &result, std::uint64_t now);
	virtual void          troops_attack(boost::shared_ptr<DungeonObject> target,bool passive = false);
	virtual std::uint64_t on_attack(boost::shared_ptr<DungeonObject> attacker,std::uint64_t demage);
	virtual std::uint64_t patrol();
	virtual std::uint64_t on_lose_target();
public:
	std::uint64_t          get_ai_id();
public:
	std::uint64_t              m_unique_id;
	boost::weak_ptr<DungeonObject> m_parent_object;
};


class AiControlMonsterCommon : public AiControl {
public:
	AiControlMonsterCommon(std::uint64_t unique_id,boost::weak_ptr<DungeonObject> parent);
	~AiControlMonsterCommon();
public:
	std::uint64_t move(std::pair<long, std::string> &result) override;
	std::uint64_t on_lose_target() override;
};

}

#endif
