#ifndef EMPERY_CLUSTER_MAP_OBJECT_HPP_
#define EMPERY_CLUSTER_MAP_OBJECT_HPP_

#include <poseidon/virtual_shared_from_this.hpp>
#include <poseidon/fwd.hpp>
#include <boost/container/flat_map.hpp>
#include <deque>
#include "id_types.hpp"
#include "coord.hpp"

namespace EmperyCluster {

class MapObject;
class AiControl{
public:
	AiControl(boost::weak_ptr<MapObject> parent);
public:
	std::uint64_t attack(std::pair<long, std::string> &result, std::uint64_t now);
	void          troops_attack();
	std::uint64_t on_attack(boost::shared_ptr<MapObject> attacker,std::uint64_t demage);
	std::uint64_t on_die(boost::shared_ptr<MapObject> attacker);
private:
	boost::weak_ptr<MapObject> m_parent_object;
};

class ClusterClient;

class MapObject : public virtual Poseidon::VirtualSharedFromThis {
public:
	enum Action {
		ACT_GUARD                      = 0,
		ACT_ATTACK                     = 1,
		ACT_DEPLOY_INTO_CASTLE         = 2,
		ACT_HARVEST_OVERLAY            = 3,
		ACT_ENTER_CASTLE               = 4,
		ACT_HARVEST_STRATEGIC_RESOURCE = 5,
	};

	enum Attackimpact {
		IMPACT_NORMAL            = 1,
		IMPACT_MISS              = 2,
		IMPACT_CRITICAL          = 3,
	};

	struct Waypoint {
		std::uint64_t delay; // 毫秒
		signed char dx;      // 相对坐标 X
		signed char dy;      // 相对坐标 Y

		Waypoint(std::uint64_t delay_, signed char dx_, signed char dy_)
			: delay(delay_), dx(dx_), dy(dy_)
		{
		}
	};

private:
	const MapObjectUuid m_map_object_uuid;
	const MapObjectTypeId m_map_object_type_id;
	const AccountUuid m_owner_uuid;
	const MapObjectUuid m_parent_object_uuid;
	const bool m_garrisoned;
	const boost::weak_ptr<ClusterClient> m_cluster;

	Coord m_coord;
	boost::container::flat_map<AttributeId, std::int64_t> m_attributes;

	boost::shared_ptr<Poseidon::TimerItem> m_action_timer;
	std::uint64_t m_next_action_time = 0;
	// 移动。
	std::deque<Waypoint> m_waypoints;
	unsigned m_blocked_retry_count = 0;
	// 移动完毕后动作。
	Action m_action = ACT_GUARD;
	std::string m_action_param;
	
	boost::shared_ptr<AiControl> m_ai_control;

public:
	MapObject(MapObjectUuid map_object_uuid, MapObjectTypeId map_object_type_id,
		AccountUuid owner_uuid, MapObjectUuid parent_object_uuid,bool garrisoned, boost::weak_ptr<ClusterClient> cluster,
		Coord coord, boost::container::flat_map<AttributeId, std::int64_t> attributes);
	~MapObject();

private:
	// 返回下一个动作的延迟。如果返回 UINT64_MAX 则当前动作被取消。
	std::uint64_t pump_action(std::pair<long, std::string> &result, std::uint64_t now);
public:
	bool is_die();
	bool is_in_attack_scope(MapObjectUuid target_uuid);
	bool is_in_group_view_scope(boost::shared_ptr<MapObject>& target_object);
public:
	boost::shared_ptr<AiControl> require_ai_control();
	std::uint64_t move(std::pair<long, std::string> &result);
	std::uint64_t attack(std::pair<long, std::string> &result, std::uint64_t now);
	void          troops_attack();
	std::uint64_t on_attack(boost::shared_ptr<MapObject> attacker,std::uint64_t demage);
	std::uint64_t on_die(boost::shared_ptr<MapObject> attacker);
private:
	void          notify_way_points(std::deque<Waypoint> &waypoints,MapObject::Action &action, std::string &action_param);
	void          fix_attack_action();
	bool          find_way_points(std::deque<Waypoint> &waypoints,Coord from_coord,Coord target_coord);
public:
	MapObjectUuid get_map_object_uuid() const {
		return m_map_object_uuid;
	}
	MapObjectTypeId get_map_object_type_id() const {
		return m_map_object_type_id;
	}
	AccountUuid get_owner_uuid() const {
		return m_owner_uuid;
	}
	MapObjectUuid get_parent_object_uuid() const {
		return m_parent_object_uuid;
	}
	bool is_garrisoned() const{
		return m_garrisoned;
	}
	boost::shared_ptr<ClusterClient> get_cluster() const {
		return m_cluster.lock();
	}

	Coord get_coord() const;
	void set_coord(Coord coord);

	std::int64_t get_attribute(AttributeId map_object_attr_id) const;
	void get_attributes(boost::container::flat_map<AttributeId, std::int64_t> &ret) const;
	void set_attributes(const boost::container::flat_map<AttributeId, std::int64_t> &modifiers);

	bool is_moving() const {
		return !m_waypoints.empty();
	}
	
	bool is_idle() const {
		return (m_action == ACT_GUARD)&&(m_waypoints.empty());
	}
	void set_action(Coord from_coord, std::deque<Waypoint> waypoints, Action action, std::string action_param);
};

}

#endif
