#include "precompiled.hpp"
#include "map_object.hpp"
#include <poseidon/singletons/timer_daemon.hpp>
#include "cluster_client.hpp"
#include "singletons/world_map.hpp"
#include "checked_arithmetic.hpp"
#include "utilities.hpp"
#include "map_cell.hpp"
#include "data/map.hpp"
#include "../../empery_center/src/map_object_type_ids.hpp"

namespace EmperyCluster {

namespace {
	enum BlockedResult {
		BR_UNBLOCKED     = 0,
		BR_BLOCKED_PERMA = 1,
		BR_BLOCKED_TEMP  = 2,
	};
}

MapObject::MapObject(MapObjectUuid map_object_uuid, MapObjectTypeId map_object_type_id, AccountUuid owner_uuid,
	Coord coord, boost::container::flat_map<AttributeId, boost::int64_t> attributes)
	: m_map_object_uuid(map_object_uuid), m_map_object_type_id(map_object_type_id), m_owner_uuid(owner_uuid)
	, m_coord(coord), m_attributes(std::move(attributes))
	, m_next_action_time(0)
	, m_blocked_retry_count(0)
{
}
MapObject::~MapObject(){
}

unsigned MapObject::is_blocked(Coord new_coord) const {
	PROFILE_ME;

	const auto owner_uuid = get_owner_uuid();
	const auto new_map_cell = WorldMap::get_map_cell(new_coord);
	if(new_map_cell){
		const auto cell_owner_uuid = new_map_cell->get_owner_uuid();
		if(cell_owner_uuid && (owner_uuid != cell_owner_uuid)){
			LOG_EMPERY_CLUSTER_DEBUG("Blocked by a cell owned by another player: cell_owner_uuid = ", cell_owner_uuid);
			return BR_BLOCKED_PERMA;
		}
	}

	const auto new_cluster_and_scope = WorldMap::get_cluster_and_scope(new_coord);
	const auto &new_cluster = new_cluster_and_scope.first;
	if(!new_cluster){
		LOG_EMPERY_CLUSTER_DEBUG("Lost connection to center server.");
		return BR_BLOCKED_PERMA;
	}
	const auto &new_scope = new_cluster_and_scope.second;
	const auto map_x = static_cast<unsigned>(new_coord.x() - new_scope.left());
	const auto map_y = static_cast<unsigned>(new_coord.y() - new_scope.bottom());
	const auto cell_data = Data::MapCellBasic::require(map_x, map_y);
	const auto terrain_data = Data::MapCellTerrain::require(cell_data->terrain_id);
	if(!terrain_data->passable){
		LOG_EMPERY_CLUSTER_DEBUG("Blocked by terrain: terrain_id = ", terrain_data->terrain_id);
		return BR_BLOCKED_PERMA;
	}

	std::vector<boost::shared_ptr<MapObject>> adjacent_objects;
	WorldMap::get_map_objects_by_rectangle(adjacent_objects,
		Rectangle(Coord(new_coord.x() - 3, new_coord.y() - 3), Coord(new_coord.x() + 3, new_coord.y() + 3)));
	std::vector<Coord> foundation;
	for(auto it = adjacent_objects.begin(); it != adjacent_objects.end(); ++it){
		const auto &other_object = *it;
		const auto other_map_object_uuid = other_object->get_map_object_uuid();
		const auto other_coord = other_object->get_coord();
		if(new_coord == other_coord){
			LOG_EMPERY_CLUSTER_DEBUG("Blocked by another map object: other_map_object_uuid = ", other_map_object_uuid);
			if(!other_object->m_waypoints.empty()){
				return BR_BLOCKED_TEMP;
			}
			return BR_BLOCKED_PERMA;
		}
		if(other_object->get_map_object_type_id() != EmperyCenter::MapObjectTypeIds::ID_CASTLE){
			continue;
		}
		foundation.clear();
		get_castle_foundation(foundation, other_coord);
		for(auto fit = foundation.begin(); fit != foundation.end(); ++fit){
			if(new_coord == *fit){
				LOG_EMPERY_CLUSTER_DEBUG("Blocked by castle: other_map_object_uuid = ", other_map_object_uuid);
				return BR_BLOCKED_PERMA;
			}
		}
	}

	return BR_UNBLOCKED;
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

void MapObject::set_action(Coord from_coord, std::deque<Waypoint> waypoints, MapObjectUuid attack_target_uuid){
	PROFILE_ME;

	const auto timer_proc = [&](const boost::weak_ptr<MapObject> &weak, boost::uint64_t now){
		PROFILE_ME;

		const auto shared = weak.lock();
		if(!shared){
			return;
		}
		const auto map_object_uuid = get_map_object_uuid();
		LOG_EMPERY_CLUSTER_TRACE("Map object action timer: map_object_uuid = ", map_object_uuid);

		for(;;){
			if(now < m_next_action_time){
				if(m_action_timer){
					Poseidon::TimerDaemon::set_absolute_time(m_action_timer, m_next_action_time);
				}
				break;
			}
//=============================================================================
	if(!m_waypoints.empty()){
		const auto waypoint = m_waypoints.front();
		const auto old_coord = get_coord();
		const auto new_coord = Coord(old_coord.x() + waypoint.dx, old_coord.y() + waypoint.dy);

		auto blocked_result = is_blocked(new_coord);
		if(blocked_result == BR_BLOCKED_TEMP){
			const auto retry_max_count = get_config<unsigned>("blocked_path_retry_max_count", 10);
			LOG_EMPERY_CLUSTER_DEBUG("Path is blocked: map_object_uuid = ", map_object_uuid, ", new_coord = ", new_coord,
				", retry_max_count = ", retry_max_count, ", retry_count = ", m_blocked_retry_count);
			if(m_blocked_retry_count >= retry_max_count){
				LOG_EMPERY_CLUSTER_DEBUG("Give up the path.");
				blocked_result = BR_BLOCKED_PERMA;
			}
		}
		if(blocked_result != BR_UNBLOCKED){
			if(blocked_result == BR_BLOCKED_PERMA){
				m_waypoints.clear();
				m_blocked_retry_count = 0;
				continue;
			}
			LOG_EMPERY_CLUSTER_DEBUG("Will retry later.");

			const auto retry_delay = get_config<boost::uint64_t>("blocked_path_retry_delay", 500);

			++m_blocked_retry_count;
			m_next_action_time = saturated_add(m_next_action_time, retry_delay);
			continue;
		}

		LOG_EMPERY_CLUSTER_DEBUG("Setting new coord: map_object_uuid = ", map_object_uuid, ", new_coord = ", new_coord);
		set_coord(new_coord);
		m_waypoints.pop_front();
		m_blocked_retry_count = 0;
		m_next_action_time = saturated_add(m_next_action_time, waypoint.delay);
		continue;
	}

	if(m_attack_target_uuid){
		const auto target = WorldMap::get_map_object(m_attack_target_uuid);
		if(!target){
			LOG_EMPERY_CLUSTER_DEBUG("Lost target: map_object_uuid = ", map_object_uuid, ", target_uuid = ", m_attack_target_uuid);
			m_attack_target_uuid = { };
			continue;
		}
		// TODO 战斗。
		LOG_EMPERY_CLUSTER_FATAL("=== BATTLE ===");
		m_attack_target_uuid = { };
		m_next_action_time = saturated_add(m_next_action_time, (boost::uint64_t)1000);
		continue;
	}
//=============================================================================
			LOG_EMPERY_CLUSTER_DEBUG("Releasing action timer: map_object_uuid = ", map_object_uuid);
			m_action_timer.reset();
			break;
		}
	};

	m_waypoints.clear();
	m_attack_target_uuid = MapObjectUuid();

	set_coord(from_coord);

	const auto now = Poseidon::get_fast_mono_clock();

	if(!m_action_timer && (!waypoints.empty() || attack_target_uuid)){
		auto timer = Poseidon::TimerDaemon::register_absolute_timer(now, 1000,
			std::bind(timer_proc, virtual_weak_from_this<MapObject>(), std::placeholders::_2));
		LOG_EMPERY_CLUSTER_DEBUG("Created action timer: map_object_uuid = ", get_map_object_uuid());
		m_action_timer = std::move(timer);
	}
	m_next_action_time = now;

	m_waypoints = std::move(waypoints);
	m_attack_target_uuid = attack_target_uuid;
}

}
