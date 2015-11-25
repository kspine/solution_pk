#include "precompiled.hpp"
#include "map_object.hpp"
#include <poseidon/singletons/timer_daemon.hpp>
#include "cluster_client.hpp"
#include "singletons/world_map.hpp"
#include "checked_arithmetic.hpp"

namespace EmperyCluster {

MapObject::MapObject(MapObjectUuid map_object_uuid, MapObjectTypeId map_object_type_id, AccountUuid owner_uuid,
	Coord coord, boost::container::flat_map<AttributeId, boost::int64_t> attributes)
	: m_map_object_uuid(map_object_uuid), m_map_object_type_id(map_object_type_id), m_owner_uuid(owner_uuid)
	, m_coord(coord), m_attributes(std::move(attributes))
	, m_last_step_time(0)
{
}
MapObject::~MapObject(){
}

void MapObject::setup_timer(){
	PROFILE_ME;

	if(!m_timer){
		m_timer = Poseidon::TimerDaemon::register_timer(0, 100,
			std::bind([](const boost::weak_ptr<MapObject> &weak, boost::uint64_t now){
				PROFILE_ME;
				const auto map_object = weak.lock();
				if(!map_object){
					return;
				}
				if(!map_object->on_timer(now)){
					LOG_EMPERY_CLUSTER_DEBUG("Releasing timer: map_object_uuid = ", map_object->get_map_object_uuid());
					map_object->m_timer.reset();
				}
			}, virtual_weak_from_this<MapObject>(), std::placeholders::_2));
		LOG_EMPERY_CLUSTER_DEBUG("Created timer: map_object_uuid = ", get_map_object_uuid());
	} else {
		Poseidon::TimerDaemon::set_absolute_time(m_timer, 0);
	}
}
bool MapObject::on_timer(boost::uint64_t now){
	PROFILE_ME;
	LOG_EMPERY_CLUSTER_TRACE("Map object timer: map_object_uuid = ", get_map_object_uuid());

	bool preserves_timer = false;

	// 检查移动。
	auto waypoints_erased_end = m_waypoints.begin();
	auto due_time = m_last_step_time;
	auto coord = get_coord();
	for(;;){
		if(waypoints_erased_end == m_waypoints.end()){
			break;
		}
		const auto &waypoint = *waypoints_erased_end;
		preserves_timer = true;

		const auto new_due_time = saturated_add(due_time, waypoint.delay);
		if(now < new_due_time){
			break;
		}
		const auto new_coord = Coord(coord.x() + waypoint.dx, coord.y() + waypoint.dy);

		++waypoints_erased_end;
		due_time = new_due_time;
		coord = new_coord;
	}
	if(waypoints_erased_end != m_waypoints.begin()){
		LOG_EMPERY_CLUSTER_DEBUG("Setting new coord: map_object_uuid = ", get_map_object_uuid(), ", coord = ", coord);
		set_coord(coord);

		m_last_step_time = now;
		m_waypoints.erase(m_waypoints.begin(), waypoints_erased_end);
	}

	return preserves_timer;
}

Coord MapObject::get_coord() const {
	return m_coord;
}
void MapObject::set_coord(Coord coord){
	PROFILE_ME;

	if(get_coord() == coord){
		return;
	}
	m_coord = coord;

	WorldMap::update_map_object(virtual_shared_from_this<MapObject>(), false);
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
		m_attributes.emplace(it->first, 0);
	}

	bool dirty = false;
	for(auto it = modifiers.begin(); it != modifiers.end(); ++it){
		auto &value = m_attributes.at(it->first);
		if(value == it->second){
			continue;
		}
		value = it->second;
		++dirty;
	}
	if(!dirty){
	    return;
	}

	WorldMap::update_map_object(virtual_shared_from_this<MapObject>(), false);
}

void MapObject::set_waypoints(Coord from_coord, std::deque<Waypoint> waypoints){
	PROFILE_ME;

	set_coord(from_coord);

	setup_timer();

	m_last_step_time = Poseidon::get_fast_mono_clock();
	m_waypoints = std::move(waypoints);
}

void MapObject::set_attack_target_uuid(MapObjectUuid attack_target_uuid){
	PROFILE_ME;

	m_attack_target_uuid = attack_target_uuid;
}

}
