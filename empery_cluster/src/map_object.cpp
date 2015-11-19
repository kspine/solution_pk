#include "precompiled.hpp"
#include "map_object.hpp"
#include "cluster_client.hpp"
#include <poseidon/singletons/timer_daemon.hpp>

namespace EmperyCluster {

class MapObject::InvalidationGuard : NONCOPYABLE {
private:
	MapObject *const m_parent;

public:
	explicit InvalidationGuard(MapObject *parent)
		: m_parent(parent)
	{
		if(!m_parent->m_timer){
			m_parent->m_timer = Poseidon::TimerDaemon::register_timer(0, 1000,
				std::bind([](const boost::weak_ptr<MapObject> &weak, boost::uint64_t now){
					PROFILE_ME;
					const auto map_object = weak.lock();
					if(!map_object){
						return;
					}
					map_object->on_timer(now);
				}, m_parent->virtual_weak_from_this<MapObject>(), std::placeholders::_2));
			LOG_EMPERY_CLUSTER_DEBUG("Created timer: map_object_uuid = ", m_parent->get_map_object_uuid());
		} else {
			Poseidon::TimerDaemon::set_absolute_time(m_parent->m_timer, 0);
		}
	}
	~InvalidationGuard(){
		m_parent->m_invalidated = true;
	}
};

MapObject::MapObject(MapObjectUuid map_object_uuid, MapObjectTypeId map_object_type_id, AccountUuid owner_uuid,
	Coord coord, boost::container::flat_map<AttributeId, boost::int64_t> attributes)
	: m_map_object_uuid(map_object_uuid), m_map_object_type_id(map_object_type_id), m_owner_uuid(owner_uuid)
	, m_coord(coord), m_attributes(std::move(attributes))
	, m_invalidated(false)
	, m_last_step_time(0)
{
}
MapObject::~MapObject(){
}

void MapObject::on_timer(boost::uint64_t now){
	PROFILE_ME;
	LOG_EMPERY_CLUSTER_TRACE("Map object timer: map_object_uuid = ", get_map_object_uuid(), ", now = ", now);

	if(m_invalidated){
		//

		m_invalidated = false;
	}

	bool busy = false;

	//

	if(!busy){
		LOG_EMPERY_CLUSTER_DEBUG("Released timer: map_object_uuid = ", get_map_object_uuid());
		m_timer.reset();
	}
}

Coord MapObject::get_coord() const {
	return m_coord;
}
void MapObject::set_coord(Coord coord){
	PROFILE_ME;

	const InvalidationGuard guard(this);

	m_coord = coord;
}

boost::int64_t MapObject::get_attribute(AttributeId map_object_attr_id) const {
	PROFILE_ME;

	const auto it = m_attributes.find(map_object_attr_id);
	if(it == m_attributes.end()){
		return 0;
	}
	return it->second;
}
void MapObject::get_attributes(boost::container::flat_map<AttributeId, boost::int64_t> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_attributes.size());
	for(auto it = m_attributes.begin(); it != m_attributes.end(); ++it){
		ret[it->first] = it->second;
	}
}
void MapObject::set_attributes(const boost::container::flat_map<AttributeId, boost::int64_t> &modifiers){
	PROFILE_ME;

	for(auto it = modifiers.begin(); it != modifiers.end(); ++it){
		m_attributes[it->first] = 0;
	}

	const InvalidationGuard guard(this);

	for(auto it = modifiers.begin(); it != modifiers.end(); ++it){
		m_attributes.at(it->first) = it->second;
	}
}

void MapObject::set_waypoints(std::deque<Coord> waypoints){
	PROFILE_ME;

	const InvalidationGuard guard(this);

	m_last_step_time = Poseidon::get_fast_mono_clock();
	m_waypoints = std::move(waypoints);
}

}
