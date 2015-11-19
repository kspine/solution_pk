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
private:
	class InvalidationGuard;

private:
	const MapObjectUuid m_map_object_uuid;
	const MapObjectTypeId m_map_object_type_id;
	const AccountUuid m_owner_uuid;

	Coord m_coord;
	boost::container::flat_map<AttributeId, boost::int64_t> m_attributes;

	bool m_invalidated;
	boost::shared_ptr<Poseidon::TimerItem> m_timer;

	boost::uint64_t m_last_step_time;
	std::deque<Coord> m_waypoints;

public:
	MapObject(MapObjectUuid map_object_uuid, MapObjectTypeId map_object_type_id, AccountUuid owner_uuid,
		Coord coord, boost::container::flat_map<AttributeId, boost::int64_t> attributes);
	~MapObject();

private:
	void on_timer(boost::uint64_t now);

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

	void set_waypoints(std::deque<Coord> waypoints);
};

}

#endif
