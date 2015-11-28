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
	, m_next_step_time(0)
{
}
MapObject::~MapObject(){
}

void MapObject::pump_status(){
	PROFILE_ME;

	// 无事可做。
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

void MapObject::set_waypoints(Coord from_coord, std::deque<Waypoint> waypoints, MapObjectUuid attack_target_uuid){
	PROFILE_ME;

	if(!waypoints.empty()){
		const auto movement_timer_proc = [&](const boost::weak_ptr<MapObject> &weak, boost::uint64_t now){
			PROFILE_ME;

			const auto shared = weak.lock();
			if(!shared){
				return;
			}
			if(m_waypoints.empty()){
				LOG_EMPERY_CLUSTER_DEBUG("Releasing movement timer: map_object_uuid = ", get_map_object_uuid());
				m_movement_timer.reset();
				return;
			}
			LOG_EMPERY_CLUSTER_TRACE("Map object movement timer: map_object_uuid = ", get_map_object_uuid());

			while((m_next_step_time < now) && !m_waypoints.empty()){
				const auto &waypoint = m_waypoints.front();

				const auto old_coord = get_coord();
				const auto new_coord = Coord(old_coord.x() + waypoint.dx, old_coord.y() + waypoint.dy);
				const auto next_step_time = saturated_add(m_next_step_time, waypoint.delay);

				LOG_EMPERY_CLUSTER_DEBUG("Setting new coord: map_object_uuid = ", get_map_object_uuid(), ", new_coord = ", new_coord);
				set_coord(new_coord);

				m_waypoints.pop_front();
				m_next_step_time = next_step_time;
			}
		};

		auto timer = Poseidon::TimerDaemon::register_timer(0, 200,
			std::bind(movement_timer_proc, virtual_weak_from_this<MapObject>(), std::placeholders::_2));
		LOG_EMPERY_CLUSTER_DEBUG("Created movement timer: map_object_uuid = ", get_map_object_uuid());
		m_movement_timer = std::move(timer);
	}

	if(attack_target_uuid){
		const auto battle_timer_proc = [&](const boost::weak_ptr<MapObject> &weak, boost::uint64_t now){
			PROFILE_ME;

			const auto shared = weak.lock();
			if(!shared){
				return;
			}
			if(!m_attack_target_uuid){
				LOG_EMPERY_CLUSTER_DEBUG("Releasing battle timer: map_object_uuid = ", get_map_object_uuid());
				m_battle_timer.reset();
				return;
			}
			LOG_EMPERY_CLUSTER_TRACE("Map object battle timer: map_object_uuid = ", get_map_object_uuid());

			// TODO
			m_attack_target_uuid = { };
		};

		auto timer = Poseidon::TimerDaemon::register_timer(0, 1000,
			std::bind(battle_timer_proc, virtual_weak_from_this<MapObject>(), std::placeholders::_2));
		LOG_EMPERY_CLUSTER_DEBUG("Created battle timer: map_object_uuid = ", get_map_object_uuid());
		m_battle_timer = std::move(timer);
	};

	const auto now = Poseidon::get_fast_mono_clock();

	set_coord(from_coord);

	m_waypoints          = std::move(waypoints);
	m_next_step_time     = now;
	m_attack_target_uuid = attack_target_uuid;
}

}
