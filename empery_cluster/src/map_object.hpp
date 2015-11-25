#ifndef EMPERY_CLUSTER_MAP_OBJECT_HPP_
#define EMPERY_CLUSTER_MAP_OBJECT_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <poseidon/fwd.hpp>
#include <boost/container/flat_map.hpp>
#include <deque>
#include "id_types.hpp"
#include "coord.hpp"

namespace EmperyCluster {

class MapObject : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
public:
	struct Waypoint {
		boost::uint64_t delay; // 毫秒
		boost::int64_t  dx;    // 相对坐标 X
		boost::int64_t  dy;    // 相对坐标 Y

		Waypoint(boost::uint64_t delay_, boost::int64_t dx_, boost::int64_t dy_)
			: delay(delay_), dx(dx_), dy(dy_)
		{
		}
	};

private:
	const MapObjectUuid m_map_object_uuid;
	const MapObjectTypeId m_map_object_type_id;
	const AccountUuid m_owner_uuid;

	Coord m_coord;
	boost::container::flat_map<AttributeId, boost::int64_t> m_attributes;

	boost::shared_ptr<Poseidon::TimerItem> m_timer;

	// 移动。
	std::deque<Waypoint> m_waypoints;
	boost::uint64_t m_last_step_time;

	// 战斗。
	MapObjectUuid m_attack_target_uuid;

public:
	MapObject(MapObjectUuid map_object_uuid, MapObjectTypeId map_object_type_id, AccountUuid owner_uuid,
		Coord coord, boost::container::flat_map<AttributeId, boost::int64_t> attributes);
	~MapObject();

private:
	void setup_timer();
	bool on_timer(boost::uint64_t now); // 返回 false 导致计时器被删除。

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

	Coord get_coord() const;
	void set_coord(Coord coord);

	boost::int64_t get_attribute(AttributeId map_object_attr_id) const;
	void get_attributes(boost::container::flat_map<AttributeId, boost::int64_t> &ret) const;
	void set_attributes(const boost::container::flat_map<AttributeId, boost::int64_t> &modifiers);

	void set_waypoints(Coord from_coord, std::deque<Waypoint> waypoints);

	void set_attack_target_uuid(MapObjectUuid attack_target_uuid);
};

}

#endif
