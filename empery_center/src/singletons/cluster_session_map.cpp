#include "../precompiled.hpp"
#include "cluster_session_map.hpp"
#include <poseidon/multi_index_map.hpp>
#include "../cluster_session.hpp"

namespace EmperyCenter {

namespace {
	boost::uint32_t g_map_width  = 400;
	boost::uint32_t g_map_height = 300;

	struct ClusterSessionElement {
		Coord coord;
		boost::weak_ptr<ClusterSession> session;

		ClusterSessionElement(Coord coord_, boost::weak_ptr<ClusterSession> session_)
			: coord(coord_), session(std::move(session_))
		{
		}
	};

	MULTI_INDEX_MAP(ClusterSessionMapDelegator, ClusterSessionElement,
		UNIQUE_MEMBER_INDEX(coord)
		UNIQUE_MEMBER_INDEX(session)
	)

	boost::weak_ptr<ClusterSessionMapDelegator> g_session_map;

	MODULE_RAII(handles){
		get_config(g_map_width,  "map_width");
		get_config(g_map_height, "map_height");
		LOG_EMPERY_CENTER_DEBUG("> Map width = ", g_map_width, ", map height = ", g_map_height);

		const auto session_map = boost::make_shared<ClusterSessionMapDelegator>();
		g_session_map = session_map;
		handles.push(session_map);
	}
}

boost::shared_ptr<ClusterSession> ClusterSessionMap::get(Coord server_coord){
	PROFILE_ME;

	const auto session_map = g_session_map.lock();
	if(!session_map){
		LOG_EMPERY_CENTER_DEBUG("Cluster session map is not loaded.");
		return { };
	}

	const auto it = session_map->find<0>(server_coord);
	if(it == session_map->end<0>()){
		LOG_EMPERY_CENTER_DEBUG("Cluster session map is not found: server_coord = ", server_coord);
		return { };
	}
	auto session = it->session.lock();
	if(!session){
		LOG_EMPERY_CENTER_DEBUG("Session expired: server_coord = ", server_coord);
		session_map->erase<0>(it);
		return { };
	}
	return session;
}
void ClusterSessionMap::get_all(boost::container::flat_map<Coord, boost::shared_ptr<ClusterSession>> &ret){
	PROFILE_ME;

	const auto session_map = g_session_map.lock();
	if(!session_map){
		LOG_EMPERY_CENTER_DEBUG("Cluster session map is not loaded.");
		return;
	}

	ret.reserve(ret.size() + session_map->size());
	for(auto it = session_map->begin(); it != session_map->end(); ++it){
		auto session = it->session.lock();
		if(!session){
			continue;
		}
		ret.emplace(it->coord, std::move(session));
	}
}
void ClusterSessionMap::set(Coord server_coord, const boost::shared_ptr<ClusterSession> &session){
	PROFILE_ME;

	const auto session_map = g_session_map.lock();
	if(!session_map){
		LOG_EMPERY_CENTER_WARNING("Cluster session map is not loaded.");
		DEBUG_THROW(Exception, sslit("Cluster session map is not loaded"));
	}

	const auto session_it = session_map->find<1>(session);
	if(session_it != session_map->end<1>()){
		LOG_EMPERY_CENTER_WARNING("Duplicate cluster session");
		DEBUG_THROW(Exception, sslit("Duplicate cluster session"));
	}

	LOG_EMPERY_CENTER_INFO("Setting up cluster server: server_coord = ", server_coord);
	auto it = session_map->find<0>(server_coord);
	if(it == session_map->end<0>()){
		it = session_map->insert<0>(it, ClusterSessionElement(server_coord, session));
	} else {
		if(!it->session.expired()){
			LOG_EMPERY_CENTER_WARNING("Map session conflict: server_coord = ", server_coord);
			DEBUG_THROW(Exception, sslit("Map session conflict"));
		}
		session_map->set_key<0, 1>(it, session);
	}
}

Coord ClusterSessionMap::get_server_coord(const boost::weak_ptr<ClusterSession> &session){
	PROFILE_ME;

	const auto session_map = g_session_map.lock();
	if(!session_map){
		LOG_EMPERY_CENTER_WARNING("Cluster session map is not loaded.");
		return INVALID_COORD;
	}

	const auto it = session_map->find<1>(session);
	if(it == session_map->end<1>()){
		LOG_EMPERY_CENTER_DEBUG("Cluster session map is not found");
		return INVALID_COORD;
	}
	return it->coord;
}
Coord ClusterSessionMap::require_server_coord(const boost::weak_ptr<ClusterSession> &session){
	PROFILE_ME;

	auto ret = get_server_coord(session);
	if(!ret){
		DEBUG_THROW(Exception, sslit("Session not found"));
	}
	return ret;
}

Coord ClusterSessionMap::get_server_coord_from_map_coord(Coord coord){
	PROFILE_ME;

	const auto neg_inf = Coord(INT64_MIN / g_map_width,
	                           INT64_MIN / g_map_height);
	return Coord((coord.x() - neg_inf.x() * g_map_width)  / g_map_width  + neg_inf.x(),
	             (coord.y() - neg_inf.y() * g_map_height) / g_map_height + neg_inf.y());
}
Rectangle ClusterSessionMap::get_server_map_range(Coord server_coord){
	PROFILE_ME;

	const auto bottom_left = Coord(server_coord.x() * g_map_width,
	                               server_coord.y() * g_map_height);
	return Rectangle(bottom_left.x(), bottom_left.y(),
	                 g_map_width,     g_map_height);
}

}
