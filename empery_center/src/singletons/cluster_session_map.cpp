#include "../precompiled.hpp"
#include "cluster_session_map.hpp"
#include <poseidon/multi_index_map.hpp>
#include "../cluster_session.hpp"

namespace EmperyCenter {

namespace {
	boost::uint64_t g_map_width  = 400;
	boost::uint64_t g_map_height = 300;

	struct ClusterSessionElement {
		std::pair<boost::int64_t, boost::int64_t> coord;
		boost::weak_ptr<ClusterSession> session;

		ClusterSessionElement(boost::int64_t map_x_, boost::int64_t map_y_, boost::weak_ptr<ClusterSession> session_)
			: coord(map_x_, map_y_), session(std::move(session_))
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

boost::shared_ptr<ClusterSession> ClusterSessionMap::get(boost::int64_t map_x, boost::int64_t map_y){
	PROFILE_ME;

	const auto session_map = g_session_map.lock();
	if(!session_map){
		LOG_EMPERY_CENTER_DEBUG("Cluster session map is not loaded.");
		return { };
	}

	const auto it = session_map->find<0>(std::make_pair(map_x, map_y));
	if(it == session_map->end<0>()){
		LOG_EMPERY_CENTER_DEBUG("Cluster session map is not found: map_x = ", map_x, ", map_y = ", map_y);
		return { };
	}
	auto session = it->session.lock();
	if(!session){
		LOG_EMPERY_CENTER_DEBUG("Session expired: map_x = ", map_x, ", map_y = ", map_y);
		session_map->erase<0>(it);
		return { };
	}
	return session;
}
void ClusterSessionMap::get_all(ClusterSessionMap::SessionContainer &ret){
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
void ClusterSessionMap::set(boost::int64_t map_x, boost::int64_t map_y, const boost::shared_ptr<ClusterSession> &session){
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

	LOG_EMPERY_CENTER_INFO("Setting up cluster server: map_x = ", map_x, ", map_y = ", map_y);
	auto it = session_map->find<0>(std::make_pair(map_x, map_y));
	if(it == session_map->end<0>()){
		it = session_map->insert<0>(it, ClusterSessionElement(map_x, map_y, session));
	} else {
		if(!it->session.expired()){
			LOG_EMPERY_CENTER_WARNING("Map session conflict: map_x = ", map_x, ", map_y = ", map_y);
			DEBUG_THROW(Exception, sslit("Map session conflict"));
		}
		session_map->set_key<0, 1>(it, session);
	}
}

std::pair<boost::int64_t, boost::int64_t> ClusterSessionMap::require_map_coord(const boost::weak_ptr<ClusterSession> &session){
	PROFILE_ME;

	const auto session_map = g_session_map.lock();
	if(!session_map){
		LOG_EMPERY_CENTER_WARNING("Cluster session map is not loaded.");
		DEBUG_THROW(Exception, sslit("Cluster session map is not loaded"));
	}

	const auto it = session_map->find<1>(session);
	if(it == session_map->end<1>()){
		LOG_EMPERY_CENTER_WARNING("Cluster session map is not found");
		DEBUG_THROW(Exception, sslit("Cluster session map is not found"));
	}
	return it->coord;
}
std::pair<boost::uint64_t, boost::uint64_t> ClusterSessionMap::require_map_size(){
	PROFILE_ME;

	return std::make_pair(g_map_width, g_map_height);
}

}
