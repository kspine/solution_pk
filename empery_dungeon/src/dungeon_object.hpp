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

namespace EmperyDungeon {
class DungeonClient;
class Dungeon;
class DungeonObject : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
public:
	enum Action {
		ACT_GUARD                             = 0,
		ACT_ATTACK                            = 1,
		ACT_DEPLOY_INTO_CASTLE                = 2,
//		ACT_HARVEST_OVERLAY                   = 3,
		ACT_ENTER_CASTLE                      = 4,
		ACT_HARVEST_STRATEGIC_RESOURCE        = 5,
		ACT_ATTACK_TERRITORY_FORCE            = 6,
		ACT_MONTER_REGRESS             		  = 7,
		ACT_HARVEST_RESOURCE_CRATE            = 8,
		ACT_ATTACK_TERRITORY           		  = 9,
//		ACT_HARVEST_OVERLAY_FORCE             = 10,
		ACT_HARVEST_STRATEGIC_RESOURCE_FORCE  = 11,
		ACT_HARVEST_RESOURCE_CRATE_FORCE      = 12,
		ACT_STAND_BY                          = 13,
	};

	enum AttackImpact {
		IMPACT_NORMAL            = 1,
		IMPACT_MISS              = 2,
		IMPACT_CRITICAL          = 3,
	};

	enum AI {
		AI_SOLIDER                           = 5000101,
		AI_MONSTER                           = 5000201,
		AI_BUILDING                          = 5000301,
		AI_BUILDING_NO_ATTACK                = 5000302,
		AI_GOBLIN                            = 5000601,
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
	bool m_deleted = false;

	boost::container::flat_map<AttributeId, std::int64_t> m_attributes;
	boost::container::flat_map<BuffId, BuffInfo>   m_buffs;

	boost::shared_ptr<Poseidon::TimerItem> m_action_timer;
	std::uint64_t m_next_action_time = 0;
	// 移动。
	std::deque<std::pair<signed char, signed char>> m_waypoints;
	unsigned m_blocked_retry_count = 0;

	unsigned m_action = ACT_GUARD;
	std::string m_action_param;
	AccountUuid m_target_own_uuid;
	boost::shared_ptr<AiControl> m_ai_control;

public:
	DungeonObject(DungeonUuid dungeon_uuid, DungeonObjectUuid dungeon_object_uuid,
		DungeonObjectTypeId dungeon_object_type_id, AccountUuid owner_uuid,Coord coord);
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

	bool has_been_deleted() const {
		return m_deleted;
	}
	void delete_from_game() noexcept;

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
			return ((m_action == ACT_GUARD)&&(m_waypoints.empty()));
	}

	unsigned get_action() const {
		return m_action;
	}
	const std::string &get_action_param() const {
		return m_action_param;
	}
	void set_action(Coord from_coord, std::deque<std::pair<signed char, signed char>> waypoints,DungeonObject::Action action, std::string action_param);

	bool is_virtually_removed() const;

public:
	bool is_die();
	bool is_in_attack_scope(boost::shared_ptr<DungeonObject> target_object);
	bool is_in_group_view_scope(boost::shared_ptr<DungeonObject>& target_object);
	bool is_in_monster_active_scope();
	std::uint64_t get_view_range();
	std::uint64_t get_shoot_range();
	bool          get_new_enemy(AccountUuid owner_uuid,boost::shared_ptr<DungeonObject> &new_enemy_dungeon_object);
	void          attack_new_target(boost::shared_ptr<DungeonObject> enemy_dungeon_object);
	bool          attacked_able(std::pair<long, std::string> &reason);
	bool          attacking_able(std::pair<long, std::string> &reason);
	std::uint64_t search_attack();

public:
	boost::shared_ptr<AiControl> require_ai_control();
	std::uint64_t move(std::pair<long, std::string> &result);
	std::uint64_t attack(std::pair<long, std::string> &result, std::uint64_t now);
	void          troops_attack(boost::shared_ptr<DungeonObject> target, bool passive = false);
	std::uint64_t on_attack_common(boost::shared_ptr<DungeonObject> attacker,std::uint64_t demage);
	std::uint64_t lost_target_common();
	std::uint64_t lost_target_monster();
private:
	void          notify_way_points(const std::deque<std::pair<signed char, signed char>> &waypoints,const DungeonObject::Action &action, const std::string &action_param);
	bool          fix_attack_action(std::pair<long, std::string> &result);
	bool          find_way_points(std::deque<std::pair<signed char, signed char>> &waypoints,Coord from_coord,Coord target_coord,bool precise = false);
	void          monster_regress();
	bool          is_monster();
	bool          is_lost_attacked_target();
	void          reset_attack_target_own_uuid();
    AccountUuid   get_attack_target_own_uuid();
	unsigned      get_arm_attack_type();
	unsigned      get_arm_defence_type();
	int           get_attacked_prority();
	bool          move_able();
	boost::shared_ptr<const Data::DungeonObjectType> get_dungeon_object_type_data();
};

}

#endif
