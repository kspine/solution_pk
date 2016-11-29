#ifndef EMPERY_CLUSTER_MAP_OBJECT_HPP_
#define EMPERY_CLUSTER_MAP_OBJECT_HPP_

#include <poseidon/virtual_shared_from_this.hpp>
#include <poseidon/fwd.hpp>
#include <boost/container/flat_map.hpp>
#include <deque>
#include "id_types.hpp"
#include "coord.hpp"
#include "data/map_object.hpp"
#include "ai_control.hpp"

namespace EmperyCluster {

class MapObject;
class ResourceCrate;
class MapCell;

class ClusterClient;

class MapObject : public virtual Poseidon::VirtualSharedFromThis {
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
		ACT_HARVEST_LEGION_RESOURCE           = 14,
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
	const MapObjectUuid m_map_object_uuid;
	const std::uint64_t m_stamp;
	const MapObjectTypeId m_map_object_type_id;
	const AccountUuid m_owner_uuid;
	const LegionUuid m_legion_uuid;
	const MapObjectUuid m_parent_object_uuid;
	const bool m_garrisoned;
	const boost::weak_ptr<ClusterClient> m_cluster;

	Coord m_coord;
	boost::container::flat_map<AttributeId, std::int64_t> m_attributes;
	boost::container::flat_map<BuffId, BuffInfo>          m_buffs;

	boost::shared_ptr<Poseidon::TimerItem> m_action_timer;
	std::uint64_t m_next_action_time = 0;
	// 移动。
	std::deque<std::pair<signed char, signed char>> m_waypoints;
	unsigned m_blocked_retry_count = 0;
	// 移动完毕后动作。
	Action m_action = ACT_GUARD;
	std::string m_action_param;
	AccountUuid m_target_own_uuid;
	boost::shared_ptr<AiControl> m_ai_control;

public:
	MapObject(MapObjectUuid map_object_uuid, std::uint64_t stamp, MapObjectTypeId map_object_type_id,
		AccountUuid owner_uuid, LegionUuid legion_uuid, MapObjectUuid parent_object_uuid,bool garrisoned, boost::weak_ptr<ClusterClient> cluster,
		Coord coord, boost::container::flat_map<AttributeId, std::int64_t> attributes,
		boost::container::flat_map<BuffId, BuffInfo> buffs);
	~MapObject();

private:
	// 返回下一个动作的延迟。如果返回 UINT64_MAX 则当前动作被取消。
	std::uint64_t pump_action(std::pair<long, std::string> &result, std::uint64_t now);
public:
	bool is_die();
	bool is_in_attack_scope(boost::shared_ptr<MapObject> target_object);
	bool is_in_attack_scope(boost::shared_ptr<ResourceCrate> target_resource_crate);
	bool is_in_attack_scope(boost::shared_ptr<MapCell> target_territory);
	bool is_in_group_view_scope(boost::shared_ptr<MapObject>& target_object);
	bool is_in_monster_active_scope();
	std::uint64_t get_view_range();
	std::uint64_t get_shoot_range();
	bool          get_new_enemy(AccountUuid owner_uuid,boost::shared_ptr<MapObject> &new_enemy_map_object);
	void          attack_new_target(boost::shared_ptr<MapObject> enemy_map_object);
	bool          attacked_able(std::pair<long, std::string> &reason);
	bool          attacking_able(std::pair<long, std::string> &reason);
	std::uint64_t search_attack();
	bool          is_protectable();
	bool          is_level_limit(boost::shared_ptr<MapObject> enemy_map_object);
	std::int64_t get_monster_level();
public:
	boost::shared_ptr<AiControl> require_ai_control();
	std::uint64_t move(std::pair<long, std::string> &result);
	std::uint64_t attack(std::pair<long, std::string> &result, std::uint64_t now);
	void          troops_attack(boost::shared_ptr<MapObject> target, bool passive = false);
	std::uint64_t on_attack_common(boost::shared_ptr<MapObject> attacker,std::uint64_t demage);
	std::uint64_t on_attack_goblin(boost::shared_ptr<MapObject> attacker,std::uint64_t demage);
	std::uint64_t harvest_resource_crate(std::pair<long, std::string> &result, std::uint64_t now,bool forced_attack = false);
	std::uint64_t attack_territory(std::pair<long, std::string> &result, std::uint64_t now,bool forced_attack = false);
	std::uint64_t on_action_harvest_overplay(std::pair<long, std::string> &result, std::uint64_t now,bool forced_attack = false);
	std::uint64_t on_action_harvest_strategic_resource(std::pair<long, std::string> &result, std::uint64_t now,bool forced_attack = false);
	std::uint64_t on_action_harvest_legion_resource(std::pair<long, std::string> &result, std::uint64_t now,bool forced_attack = false);
	std::uint64_t on_action_harvest_resource_crate(std::pair<long, std::string> &result, std::uint64_t now,bool forced_attack = false);
	std::uint64_t on_action_attack_territory(std::pair<long, std::string> &result, std::uint64_t now,bool forced_attack = false);
	std::uint64_t lost_target_common();
	std::uint64_t lost_target_monster();
private:
	void          notify_way_points(const std::deque<std::pair<signed char, signed char>> &waypoints,const MapObject::Action &action, const std::string &action_param);
	bool          fix_attack_action(std::pair<long, std::string> &result);
	bool          find_way_points(std::deque<std::pair<signed char, signed char>> &waypoints,Coord from_coord,Coord target_coord,bool precise = false);
	void          monster_regress();
	bool          is_monster();
	bool          is_building();
	bool          is_castle();
	bool          is_bunker();
	bool          is_defense_tower();
	bool          is_legion_warehouse();
	bool          is_lost_attacked_target();
	void          reset_attack_target_own_uuid();
    AccountUuid   get_attack_target_own_uuid();
	unsigned      get_arm_attack_type();
	unsigned      get_arm_defence_type();
	int           get_attacked_prority();
	bool          move_able();
	boost::shared_ptr<const Data::MapObjectType> get_map_object_type_data();
	boost::shared_ptr<ResourceCrate> get_attack_resouce_crate();
	boost::shared_ptr<MapCell> get_attack_territory();
public:
	MapObjectUuid get_map_object_uuid() const {
		return m_map_object_uuid;
	}
	std::uint64_t get_stamp() const {
		return m_stamp;
	}
	MapObjectTypeId get_map_object_type_id() const {
		return m_map_object_type_id;
	}
	AccountUuid get_owner_uuid() const {
		return m_owner_uuid;
	}
	LegionUuid get_legion_uuid() const {
		return m_legion_uuid;
	}
	MapObjectUuid get_parent_object_uuid() const {
		return m_parent_object_uuid;
	}
	bool is_garrisoned() const {
		return m_garrisoned;
	}
	boost::shared_ptr<ClusterClient> get_cluster() const {
		return m_cluster.lock();
	}

	Coord get_coord() const;
	void set_coord(Coord coord);

	std::int64_t get_attribute(AttributeId map_object_attr_id) const;
	void get_attributes(boost::container::flat_map<AttributeId, std::int64_t> &ret) const;
	void set_attributes_no_synchronize(boost::container::flat_map<AttributeId, std::int64_t> modifiers);

	BuffInfo get_buff(BuffId buff_id) const;
	bool is_buff_in_effect(BuffId buff_id) const;
	void get_buffs(std::vector<BuffInfo> &ret) const;
	void set_buff(BuffId buff_id, std::uint64_t time_begin, std::uint64_t duration);
	void clear_buff(BuffId buff_id) noexcept;

	bool is_moving() const {
		return !m_waypoints.empty();
	}

	bool is_idle() const {
		return ((m_action == ACT_GUARD)&&(m_waypoints.empty())&& !is_garrisoned()) || (m_action == ACT_STAND_BY );
	}

	Action get_action() const {
		return m_action;
	}
	const std::string &get_action_param() const {
		return m_action_param;
	}
	void set_action(Coord from_coord, std::deque<std::pair<signed char, signed char>> waypoints, Action action, std::string action_param);
};

}

#endif
