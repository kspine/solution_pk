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
	virtual void          troops_attack(boost::shared_ptr<DungeonObject> target,bool passive = false);
	virtual std::uint64_t on_attack(boost::shared_ptr<DungeonObject> attacker,std::uint64_t demage);
	virtual std::uint64_t on_lose_target();
	virtual std::uint64_t on_action_attack(std::pair<long, std::string> &result, std::uint64_t now);
	virtual std::uint64_t on_action_monster_regress(std::pair<long, std::string> &result, std::uint64_t now);
	virtual std::uint64_t on_action_monster_search_target(std::pair<long, std::string> &result, std::uint64_t now);
	virtual std::uint64_t on_action_guard(std::pair<long, std::string> &result, std::uint64_t now);
	virtual std::uint64_t on_action_patrol(std::pair<long, std::string> &result, std::uint64_t now);
	virtual std::uint64_t on_action_skill_singing(std::pair<long, std::string> &result, std::uint64_t now);
	virtual std::uint64_t on_action_skill_casting(std::pair<long, std::string> &result, std::uint64_t now);
	virtual std::uint64_t on_action_target_move(std::pair<long, std::string> &result, std::uint64_t now);
public:
	std::uint64_t            get_ai_id();
public:
	std::uint64_t              m_unique_id;
	boost::weak_ptr<DungeonObject> m_parent_object;
};

class AiControlMonsterAutoSearch : public AiControl {
public:
	AiControlMonsterAutoSearch(std::uint64_t unique_id,boost::weak_ptr<DungeonObject> parent);
	~AiControlMonsterAutoSearch();
public:
	std::uint64_t on_lose_target() override;
	std::uint64_t on_action_monster_regress(std::pair<long, std::string> &result, std::uint64_t now) override;
	std::uint64_t on_action_monster_search_target(std::pair<long, std::string> &result, std::uint64_t now) override;
	std::uint64_t on_action_guard(std::pair<long, std::string> &result, std::uint64_t now) override;
};

class AiControlMonsterPatrol : public AiControl {
public:
	AiControlMonsterPatrol(std::uint64_t unique_id,boost::weak_ptr<DungeonObject> parent);
	~AiControlMonsterPatrol();
public:
	std::uint64_t move(std::pair<long, std::string> &result) override;
	std::uint64_t on_lose_target() override;
	std::uint64_t on_action_monster_search_target(std::pair<long, std::string> &result, std::uint64_t now) override;
	std::uint64_t on_action_guard(std::pair<long, std::string> &result, std::uint64_t now) override;
	std::uint64_t on_action_patrol(std::pair<long, std::string> &result, std::uint64_t now) override;
};

class AiControlMonsterObject : public AiControl {
public:
	AiControlMonsterObject(std::uint64_t unique_id,boost::weak_ptr<DungeonObject> parent);
	~AiControlMonsterObject();
public:
	std::uint64_t move(std::pair<long, std::string> &result) override;
	std::uint64_t on_attack(boost::shared_ptr<DungeonObject> attacker,std::uint64_t demage) override;
	void          troops_attack(boost::shared_ptr<DungeonObject> target,bool passive = false) override;
};

class AiControlMonsterDecorate : public AiControl {
public:
	AiControlMonsterDecorate(std::uint64_t unique_id,boost::weak_ptr<DungeonObject> parent);
	~AiControlMonsterDecorate();
public:
	std::uint64_t on_action_attack(std::pair<long, std::string> &result, std::uint64_t now) override;
	std::uint64_t move(std::pair<long, std::string> &result) override;
	std::uint64_t on_attack(boost::shared_ptr<DungeonObject> attacker,std::uint64_t demage) override;
	void          troops_attack(boost::shared_ptr<DungeonObject> target,bool passive = false) override;
};

}

#endif
