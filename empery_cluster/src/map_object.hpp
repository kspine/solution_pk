#ifndef EMPERY_CLUSTER_MAP_OBJECT_HPP_
#define EMPERY_CLUSTER_MAP_OBJECT_HPP_

#include <poseidon/virtual_shared_from_this.hpp>
#include <poseidon/fwd.hpp>
#include <boost/container/flat_map.hpp>
#include <deque>
#include "id_types.hpp"
#include "coord.hpp"

namespace EmperyCluster {

class ClusterClient;

class MapObject : public virtual Poseidon::VirtualSharedFromThis {
public:
	enum Action {
		ACT_GUARD               = 0,
		ACT_ATTACK              = 1,
		ACT_DEPLOY_INTO_CASTLE  = 2,
		ACT_HARVEST_OVERLAY     = 3,
	};

	struct Waypoint {
		std::uint64_t delay;  // 毫秒
		std::int64_t dx;      // 相对坐标 X
		std::int64_t dy;      // 相对坐标 Y

		Waypoint(std::uint64_t delay_, std::int64_t dx_, boost::int64_t dy_)
			: delay(delay_), dx(dx_), dy(dy_)
		{
		}
	};

private:
	const MapObjectUuid m_map_object_uuid;
	const MapObjectTypeId m_map_object_type_id;
	const AccountUuid m_owner_uuid;
	const boost::weak_ptr<ClusterClient> m_cluster;

	Coord m_coord;
	boost::container::flat_map<AttributeId, std::int64_t> m_attributes;

	boost::shared_ptr<Poseidon::TimerItem> m_action_timer;
	std::uint64_t m_next_action_time;
	// 移动。
	std::deque<Waypoint> m_waypoints;
	unsigned m_blocked_retry_count;
	// 移动完毕后动作。
	Action m_action;
	std::string m_action_param;

public:
	MapObject(MapObjectUuid map_object_uuid, MapObjectTypeId map_object_type_id, AccountUuid owner_uuid,
		boost::weak_ptr<ClusterClient> cluster, Coord coord, boost::container::flat_map<AttributeId, std::int64_t> attributes);
	~MapObject();

private:
	// 返回下一个动作的延迟。如果返回 UINT64_MAX 则当前动作被取消。
	std::uint64_t pump_action(std::pair<long, std::string> &result, boost::uint64_t now);

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
	boost::shared_ptr<ClusterClient> get_cluster() const {
		return m_cluster.lock();
	}

	Coord get_coord() const;
	void set_coord(Coord coord);

	std::int64_t get_attribute(AttributeId map_object_attr_id) const;
	void get_attributes(boost::container::flat_map<AttributeId, std::int64_t> &ret) const;
	void set_attributes(const boost::container::flat_map<AttributeId, std::int64_t> &modifiers);

	void set_action(Coord from_coord, std::deque<Waypoint> waypoints, Action action, std::string action_param);
};

}

#endif
