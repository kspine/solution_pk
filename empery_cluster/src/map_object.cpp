#include "precompiled.hpp"
#include "map_object.hpp"
#include <poseidon/singletons/timer_daemon.hpp>
#include <poseidon/cbpp/status_codes.hpp>
#include "cluster_client.hpp"
#include "singletons/world_map.hpp"
#include "checked_arithmetic.hpp"
#include "utilities.hpp"
#include "map_cell.hpp"
#include "data/map.hpp"
#include "../../empery_center/src/msg/sc_map.hpp"
#include "../../empery_center/src/msg/ks_map.hpp"
#include "../../empery_center/src/msg/err_map.hpp"
#include "../../empery_center/src/map_object_type_ids.hpp"
#include "../../empery_center/src/cbpp_response.hpp"

namespace EmperyCluster {

namespace Msg {
	using namespace ::EmperyCenter::Msg;
}

using Response = ::EmperyCenter::CbppResponse;

MapObject::MapObject(MapObjectUuid map_object_uuid, MapObjectTypeId map_object_type_id, AccountUuid owner_uuid,
	boost::weak_ptr<ClusterClient> cluster, Coord coord, boost::container::flat_map<AttributeId, std::int64_t> attributes)
	: m_map_object_uuid(map_object_uuid), m_map_object_type_id(map_object_type_id), m_owner_uuid(owner_uuid)
	, m_cluster(std::move(cluster)), m_coord(coord), m_attributes(std::move(attributes))
	, m_next_action_time(0), m_blocked_retry_count(0)
	, m_action(ACT_GUARD), m_action_param()
{
}
MapObject::~MapObject(){
}

std::uint64_t MapObject::pump_action(std::pair<long, std::string> &result, std::uint64_t /* now */){
	PROFILE_ME;

	const auto map_object_uuid = get_map_object_uuid();
	const auto owner_uuid      = get_owner_uuid();
	const auto coord           = get_coord();

	// 移动。
	if(!m_waypoints.empty()){
		const auto waypoint  = m_waypoints.front();
		const auto new_coord = Coord(coord.x() + waypoint.dx, coord.y() + waypoint.dy);
		const auto delay     = waypoint.delay;

		// 检测阻挡。
		const auto new_map_cell = WorldMap::get_map_cell(new_coord);
		if(new_map_cell){
			const auto cell_owner_uuid = new_map_cell->get_owner_uuid();
			if(cell_owner_uuid && (owner_uuid != cell_owner_uuid)){
				LOG_EMPERY_CLUSTER_DEBUG("Blocked by a cell owned by another player's territory: cell_owner_uuid = ", cell_owner_uuid);
				result = Response(Msg::ERR_BLOCKED_BY_OTHER_TERRITORY) <<cell_owner_uuid;
				return UINT64_MAX;
			}
		}

		const auto new_cluster = WorldMap::get_cluster(new_coord);
		if(!new_cluster){
			LOG_EMPERY_CLUSTER_DEBUG("Lost connection to center server: new_coord = ", new_coord);
			result = Response(Msg::ERR_CLUSTER_CONNECTION_LOST) <<new_coord;
			return UINT64_MAX;
		}

		const auto new_cluster_scope = WorldMap::get_cluster_scope(new_coord);
		const auto map_x = static_cast<unsigned>(new_coord.x() - new_cluster_scope.left());
		const auto map_y = static_cast<unsigned>(new_coord.y() - new_cluster_scope.bottom());
		const auto cell_data = Data::MapCellBasic::require(map_x, map_y);
		const auto terrain_id = cell_data->terrain_id;
		const auto terrain_data = Data::MapTerrain::require(terrain_id);
		if(!terrain_data->passable){
			LOG_EMPERY_CLUSTER_DEBUG("Blocked by terrain: terrain_id = ", terrain_id);
			result = Response(Msg::ERR_BLOCKED_BY_IMPASSABLE_MAP_CELL) <<terrain_id;
			return UINT64_MAX;
		}

		std::vector<boost::shared_ptr<MapObject>> adjacent_objects;
		WorldMap::get_map_objects_by_rectangle(adjacent_objects,
			Rectangle(Coord(new_coord.x() - 3, new_coord.y() - 3), Coord(new_coord.x() + 4, new_coord.y() + 4)));
		std::vector<Coord> foundation;
		for(auto it = adjacent_objects.begin(); it != adjacent_objects.end(); ++it){
			const auto &other_object = *it;
			const auto other_map_object_uuid = other_object->get_map_object_uuid();
			const auto other_coord = other_object->get_coord();
			if(new_coord == other_coord){
				LOG_EMPERY_CLUSTER_DEBUG("Blocked by another map object: other_map_object_uuid = ", other_map_object_uuid);
				if(!other_object->m_waypoints.empty()){
					const auto retry_max_count = get_config<unsigned>("blocked_path_retry_max_count", 10);
					const auto retry_delay = get_config<std::uint64_t>("blocked_path_retry_delay", 500);
					LOG_EMPERY_CLUSTER_DEBUG("Should we retry? blocked_retry_count = ", m_blocked_retry_count,
						", retry_max_count = ", retry_max_count, ", retry_delay = ", retry_delay);
					if(m_blocked_retry_count < retry_max_count){
						++m_blocked_retry_count;
						return retry_delay;
					}
					LOG_EMPERY_CLUSTER_DEBUG("Give up the path.");
				}
				result = Response(Msg::ERR_BLOCKED_BY_TROOPS) <<other_map_object_uuid;
				return UINT64_MAX;
			}
			const auto other_object_type_id = other_object->get_map_object_type_id();
			if(other_object_type_id != EmperyCenter::MapObjectTypeIds::ID_CASTLE){
				continue;
			}
			foundation.clear();
			get_castle_foundation(foundation, other_coord, false);
			for(auto fit = foundation.begin(); fit != foundation.end(); ++fit){
				if(new_coord == *fit){
					LOG_EMPERY_CLUSTER_DEBUG("Blocked by castle: other_map_object_uuid = ", other_map_object_uuid);
					result = Response(Msg::ERR_BLOCKED_BY_CASTLE) <<other_map_object_uuid;
					return UINT64_MAX;
				}
			}
		}

		LOG_EMPERY_CLUSTER_DEBUG("Setting new coord: map_object_uuid = ", map_object_uuid, ", new_coord = ", new_coord);
		set_coord(new_coord);

		m_waypoints.pop_front();
		m_blocked_retry_count = 0;

		return delay;
	}

	switch(m_action){
		{
#define ON_ACTION(action_)	\
		}	\
		break;	\
	case (action_): {
//=============================================================================
	ON_ACTION(ACT_GUARD){
		// 无事可做。
	}
	ON_ACTION(ACT_ATTACK){
		const auto target_object_uuid = MapObjectUuid(m_action_param);
		const auto target_object = WorldMap::get_map_object(target_object_uuid);
		if(!target_object){
			break;
		}
		// TODO 战斗。
	}
	ON_ACTION(ACT_DEPLOY_INTO_CASTLE){
		const auto cluster = get_cluster();
		if(!cluster){
			break;
		}
		Msg::KS_MapDeployImmigrants sreq;
		sreq.map_object_uuid = map_object_uuid.str();
		sreq.castle_name     = m_action_param;
		auto sresult = cluster->send_and_wait(sreq);
		if(sresult.first != Msg::ST_OK){
			LOG_EMPERY_CLUSTER_DEBUG("Center server returned an error: code = ", sresult.first, ", msg = ", sresult.second);
			result = std::move(sresult);
			break;
		}
	}
	ON_ACTION(ACT_HARVEST_OVERLAY){
		const auto harvest_interval = get_config<std::uint64_t>("harvest_interval", 1000);
		const auto cluster = get_cluster();
		if(!cluster){
			break;
		}
		Msg::KS_MapHarvestOverlay sreq;
		sreq.map_object_uuid = map_object_uuid.str();
		sreq.interval        = harvest_interval;
		auto sresult = cluster->send_and_wait(sreq);
		if(sresult.first != Msg::ST_OK){
			LOG_EMPERY_CLUSTER_DEBUG("Center server returned an error: code = ", sresult.first, ", msg = ", sresult.second);
			result = std::move(sresult);
			break;
		}
		return harvest_interval;
	}
//=============================================================================
#undef ON_ACTION
		}
		break;
	default:
		LOG_EMPERY_CLUSTER_WARNING("Unknown action: action = ", static_cast<unsigned>(m_action));
		result = Response(Msg::ERR_UNKNOWN_MAP_OBJECT_ACTION) <<static_cast<unsigned>(m_action);
		break;
	}
	return UINT64_MAX;
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

std::int64_t MapObject::get_attribute(AttributeId map_object_attr_id) const {
	PROFILE_ME;

	const auto it = m_attributes.find(map_object_attr_id);
	if(it == m_attributes.end()){
		return 0;
	}
	return it->second;
}
void MapObject::get_attributes(boost::container::flat_map<AttributeId, std::int64_t> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_attributes.size());
	for(auto it = m_attributes.begin(); it != m_attributes.end(); ++it){
		ret[it->first] = it->second;
	}
}
void MapObject::set_attributes(const boost::container::flat_map<AttributeId, std::int64_t> &modifiers){
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

void MapObject::set_action(Coord from_coord, std::deque<Waypoint> waypoints, MapObject::Action action, std::string action_param){
	PROFILE_ME;

	const auto timer_proc = [this](const boost::weak_ptr<MapObject> &weak, std::uint64_t now){
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

			std::uint64_t delay = UINT64_MAX;
			std::pair<long, std::string> result;
			try {
				delay = pump_action(result, now);
			} catch(std::exception &e){
				LOG_EMPERY_CLUSTER_WARNING("std::exception thrown: what = ", e.what());
				result.first = Msg::ERR_INVALID_ACTION_PARAM;
				try {
					result.second = e.what();
				} catch(std::exception &e){
					LOG_EMPERY_CLUSTER_ERROR("std::exception thrown: what = ", e.what());
				}
			}
			if(delay == UINT64_MAX){
				auto old_action = m_action;
				auto old_param  = std::move(m_action_param);

				LOG_EMPERY_CLUSTER_DEBUG("Action stopped: map_object_uuid = ", map_object_uuid,
					", error_code = ", result.first, ", error_message = ", result.second);
				m_waypoints.clear();
				m_action = ACT_GUARD;
				m_action_param.clear();

				LOG_EMPERY_CLUSTER_DEBUG("Releasing action timer: map_object_uuid = ", map_object_uuid);
				m_action_timer.reset();

				const auto cluster = get_cluster();
				if(cluster){
					try {
						Msg::SC_MapObjectStopped msg;
						msg.map_object_uuid = map_object_uuid.str();
						msg.action          = old_action;
						msg.param           = std::move(old_param);
						msg.error_code      = result.first;
						msg.error_message   = std::move(result.second);
						cluster->send_notification(get_owner_uuid(), msg);
					} catch(std::exception &e){
						LOG_EMPERY_CLUSTER_WARNING("std::exception thrown: what = ", e.what());
						cluster->shutdown(e.what());
					}
				}
				break;
			}
			m_next_action_time = checked_add(m_next_action_time, delay);
		}
	};

	m_waypoints.clear();
	m_blocked_retry_count = 0;
	m_action = ACT_GUARD;
	m_action_param.clear();

	set_coord(from_coord);

	const auto now = Poseidon::get_fast_mono_clock();

	if(!waypoints.empty() || (action != ACT_GUARD)){
		if(!m_action_timer){
			auto timer = Poseidon::TimerDaemon::register_absolute_timer(now, 200,
				std::bind(timer_proc, virtual_weak_from_this<MapObject>(), std::placeholders::_2));
			LOG_EMPERY_CLUSTER_DEBUG("Created action timer: map_object_uuid = ", get_map_object_uuid());
			m_action_timer = std::move(timer);
		}
		if(m_next_action_time < now){
			m_next_action_time = now;
		}
	}

	m_waypoints    = std::move(waypoints);
	m_action       = action;
	m_action_param = std::move(action_param);
}

}
