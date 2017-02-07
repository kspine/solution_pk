#ifndef EMPERY_DUNGEON_DUNGEON_OBJECT_HPP_
#define EMPERY_DUNGEON_DUNGEON_OBJECT_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <boost/container/flat_map.hpp>
#include "data/dungeon_object.hpp"
#include "id_types.hpp"
#include "buff_ids.hpp"
#include "coord.hpp"
#include "dungeon.hpp"
#include "ai_control.hpp"
#include "skill.hpp"

namespace EmperyDungeon {
class DungeonClient;
class Dungeon;
class DungeonObject : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
public:
	enum Action {
		ACT_GUARD                             = 0,
		ACT_ATTACK                            = 1,
		ACT_MONTER_REGRESS                    = 2,
		ACT_MONSTER_SEARCH_TARGET             = 3,
		ACT_MONSTER_PATROL                    = 4,
		ACT_SKILL_SING                        = 5,//吟唱
		ACT_SKILL_CAST                        = 6,//施法
		ACT_TARGET_MOVE                       = 7,//移动到目标
	};

	enum AttackImpact {
		IMPACT_NORMAL            = 1,
		IMPACT_MISS              = 2,
		IMPACT_CRITICAL          = 3,
	};

	enum AI {
		AI_SOLIDER                           = 1,
		AI_MONSTER_AUTO_SEARCH_TARGET        = 4,
		AI_MONSTER_PATROL                    = 5,
		AI_MONSTER_OBJECT                    = 9,
		AI_MONSTER_DECORATE                  = 10,
	};

	enum SkillTarget {
		SKILL_TARGET_NONE                    = 0,
		SKILL_TARGET_FRIEND                  = 1,
		SKILL_TARGET_ENEMY                   = 2,
		SKILL_TARGET_GRID                    = 3,
	};
public:
	struct BuffInfo {
		BuffId buff_id;
		std::uint64_t duration;
		std::uint64_t time_begin;
		std::uint64_t time_end;
	};

private:
	const DungeonUuid m_dungeon_uuid;
	const DungeonObjectUuid m_dungeon_object_uuid;
	const DungeonObjectTypeId m_dungeon_object_type_id;
	const AccountUuid m_owner_uuid;

	Coord m_coord;
	std::string m_tag;

	boost::container::flat_map<AttributeId, std::int64_t> m_attributes;
	boost::container::flat_map<BuffId, BuffInfo>   m_buffs;

	boost::shared_ptr<Poseidon::TimerItem> m_action_timer;
	std::uint64_t m_next_action_time = 0;
	// 移动。
	std::deque<std::pair<signed char, signed char>> m_waypoints;
	unsigned m_blocked_retry_count = 0;

	Action m_action = ACT_GUARD;
	std::string m_action_param;
	AccountUuid m_target_own_uuid;
	boost::shared_ptr<AiControl> m_ai_control;
	boost::container::flat_map<DungeonMonsterSkillId, boost::shared_ptr<Skill>> m_skills;
	DungeonMonsterSkillId          m_current_skill_id;
	Coord                          m_skill_target_coord;
	std::string                    m_skill_param;
public:
	DungeonObject(DungeonUuid dungeon_uuid, DungeonObjectUuid dungeon_object_uuid,
		DungeonObjectTypeId dungeon_object_type_id, AccountUuid owner_uuid,Coord coord,std::string tag);
	~DungeonObject();

private:
	// 返回下一个动作的延迟。如果返回 UINT64_MAX 则当前动作被取消。
	std::uint64_t pump_action(std::pair<long, std::string> &result, std::uint64_t now);

public:
	DungeonUuid get_dungeon_uuid() const {
		return m_dungeon_uuid;
	}
	DungeonObjectUuid get_dungeon_object_uuid() const {
		return m_dungeon_object_uuid;
	}
	DungeonObjectTypeId get_dungeon_object_type_id() const {
		return m_dungeon_object_type_id;
	}
	AccountUuid get_owner_uuid() const {
		return m_owner_uuid;
	}

	Coord get_coord() const {
		return m_coord;
	}
	void set_coord(Coord coord);

	std::string get_tag() const {
		return m_tag;
	}
	void set_current_skill(DungeonMonsterSkillId skill_id,Coord target_coord,std::string param);
	DungeonMonsterSkillId get_current_skill_id(){
		return m_current_skill_id;
	}
	Coord       get_skill_target_coord(){
		return m_skill_target_coord;
	}
	std::string get_skill_param(){
		return m_skill_param;
	}

	std::int64_t get_attribute(AttributeId attribute_id) const;
	void get_attributes(boost::container::flat_map<AttributeId, std::int64_t> &ret) const;
	void set_attributes(boost::container::flat_map<AttributeId, std::int64_t> modifiers);
	BuffInfo get_buff(BuffId buff_id) const;
	bool is_buff_in_effect(BuffId buff_id) const;
	void get_buffs(std::vector<BuffInfo> &ret) const;
	void set_buff(BuffId buff_id, std::uint64_t duration);
	void set_buff(BuffId buff_id, std::uint64_t time_begin, std::uint64_t duration);
	void accumulate_buff(BuffId buff_id, std::uint64_t delta_duration);
	void clear_buff(BuffId buff_id) noexcept;
	bool is_moving() const{
		return !m_waypoints.empty();
	}
	bool is_idle() const {
			return m_waypoints.empty();
	}

	Action get_action() const {
		return m_action;
	}
	const std::string &get_action_param() const {
		return m_action_param;
	}
	void set_action(Coord from_coord, std::deque<std::pair<signed char, signed char>> waypoints,DungeonObject::Action action, std::string action_param);

public:
	bool is_die();
	bool is_in_attack_scope(boost::shared_ptr<DungeonObject> target_object);
	bool is_in_group_view_scope(boost::shared_ptr<DungeonObject>& target_object);
	bool          is_monster();
	std::uint64_t get_view_range();
	std::uint64_t get_shoot_range();
	//视野联动查找目标
	bool          get_new_enemy(AccountUuid owner_uuid,boost::shared_ptr<DungeonObject> &new_enemy_dungeon_object);
	//野怪待机自动搜索目标
	bool          get_monster_new_enemy(boost::shared_ptr<DungeonObject> &new_enemy_dungeon_object);
	void          attack_new_target(boost::shared_ptr<DungeonObject> enemy_dungeon_object);
	bool          attacked_able(std::pair<long, std::string> &reason);
	bool          attacking_able(std::pair<long, std::string> &reason);
	std::uint64_t search_attack();
	boost::shared_ptr<const Data::DungeonObjectType> get_dungeon_object_type_data();
	boost::shared_ptr<const Data::DungeonObjectAi>   get_dungeon_ai_data();
public:
	boost::shared_ptr<AiControl> require_ai_control();
	std::uint64_t move(std::pair<long, std::string> &result);
	std::uint64_t attack(std::pair<long, std::string> &result, std::uint64_t now);
	void          troops_attack(boost::shared_ptr<DungeonObject> target, bool passive = false);
	std::uint64_t on_attack_common(boost::shared_ptr<DungeonObject> attacker,std::uint64_t demage);
	std::uint64_t lost_target_common();
	std::uint64_t lost_target_monster();
	std::uint64_t on_monster_regress();
	std::uint64_t monster_search_attack_target(std::pair<long, std::string> &result);
	std::uint64_t on_monster_guard();
	std::uint64_t on_monster_patrol();
	std::uint64_t on_skill_singing_finish(std::pair<long, std::string> &result, std::uint64_t now);
	std::uint64_t on_skilling_casting_finish(std::pair<long, std::string> &result, std::uint64_t now);
	std::uint64_t on_action_target_move(std::pair<long, std::string> &result, std::uint64_t now);
private:
	void          notify_way_points(const std::deque<std::pair<signed char, signed char>> &waypoints,const DungeonObject::Action &action, const std::string &action_param);
	bool          fix_attack_action(std::pair<long, std::string> &result);
	bool          find_way_points(std::deque<std::pair<signed char, signed char>> &waypoints,Coord from_coord,Coord target_coord,bool precise = false);
	void          monster_regress();
	bool          is_lost_attacked_target();
	void          reset_attack_target_own_uuid();
    AccountUuid   get_attack_target_own_uuid();
	int           get_attacked_prority();
	bool          move_able();
public:
	double         get_total_defense();
	double         get_total_attack();
	double         get_move_speed();
	std::uint64_t  get_attack_delay();
	unsigned       get_arm_attack_type();
	unsigned       get_arm_defence_type();
public:
	boost::shared_ptr<Skill> create_skill(DungeonMonsterSkillId skill_id);
	bool           can_use_skill(DungeonMonsterSkillId &skill_id,std::uint64_t now);
	std::uint64_t  use_skill(DungeonMonsterSkillId skill_id,std::pair<long, std::string> &result, std::uint64_t now);
	void           check_current_skill(std::uint64_t now);
	std::uint64_t  calculate_next_skill_time(DungeonMonsterSkillId skill_id, std::uint64_t now);
	bool           choice_skill_target(DungeonMonsterSkillId skill_id,Coord &coord,DungeonObjectUuid &dungeon_object_uuid);
	std::uint64_t  do_finish_skill(DungeonMonsterSkillId skill_id,std::uint64_t now);
	void           do_skill_effects(DungeonMonsterSkillId skill_ids);
	bool           can_reflex_injury();
	void           do_reflex_injury(std::uint64_t total_damage,boost::shared_ptr<DungeonObject> attacker);
	void           do_die_skill();
public:
    void          target_move(const Coord target_coord,const std::string params);
};

}

#endif
