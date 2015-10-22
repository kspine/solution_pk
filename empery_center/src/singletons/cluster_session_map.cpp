#include "../precompiled.hpp"
#include "cluster_session_map.hpp"
#include <poseidon/multi_index_map.hpp>
#include "../cluster_session.hpp"

namespace EmperyCenter {

namespace {
	boost::uint64_t g_mapWidth  = 400;
	boost::uint64_t g_mapHeight = 300;

	struct ClusterSessionElement {
		std::pair<boost::int64_t, boost::int64_t> coord;
		boost::weak_ptr<ClusterSession> session;

		ClusterSessionElement(boost::int64_t mapX_, boost::int64_t mapY_, boost::weak_ptr<ClusterSession> session_)
			: coord(mapX_, mapY_), session(std::move(session_))
		{
		}
	};

	MULTI_INDEX_MAP(ClusterSessionMapDelegator, ClusterSessionElement,
		UNIQUE_MEMBER_INDEX(coord)
		UNIQUE_MEMBER_INDEX(session)
	)

	boost::weak_ptr<ClusterSessionMapDelegator> g_sessionMap;

	MODULE_RAII(handles){
		getConfig(g_mapWidth,  "map_width");
		getConfig(g_mapHeight, "map_height");
		LOG_EMPERY_CENTER_DEBUG("> Map width = ", g_mapWidth, ", map height = ", g_mapHeight);

		const auto sessionMap = boost::make_shared<ClusterSessionMapDelegator>();
		g_sessionMap = sessionMap;
		handles.push(sessionMap);
	}
}

boost::shared_ptr<ClusterSession> ClusterSessionMap::get(boost::int64_t mapX, boost::int64_t mapY){
	PROFILE_ME;

	const auto sessionMap = g_sessionMap.lock();
	if(!sessionMap){
		LOG_EMPERY_CENTER_DEBUG("Cluster session map is not loaded.");
		return { };
	}

	const auto it = sessionMap->find<0>(std::make_pair(mapX, mapY));
	if(it == sessionMap->end<0>()){
		LOG_EMPERY_CENTER_DEBUG("Cluster session map is not found: mapX = ", mapX, ", mapY = ", mapY);
		return { };
	}
	auto session = it->session.lock();
	if(!session){
		LOG_EMPERY_CENTER_DEBUG("Session expired: mapX = ", mapX, ", mapY = ", mapY);
		sessionMap->erase<0>(it);
		return { };
	}
	return session;
}
void ClusterSessionMap::getAll(ClusterSessionMap::SessionContainer &ret){
	PROFILE_ME;

	const auto sessionMap = g_sessionMap.lock();
	if(!sessionMap){
		LOG_EMPERY_CENTER_DEBUG("Cluster session map is not loaded.");
		return;
	}

	ret.reserve(ret.size() + sessionMap->size());
	for(auto it = sessionMap->begin(); it != sessionMap->end(); ++it){
		auto session = it->session.lock();
		if(!session){
			continue;
		}
		ret.emplace(it->coord, std::move(session));
	}
}
void ClusterSessionMap::set(boost::int64_t mapX, boost::int64_t mapY, const boost::shared_ptr<ClusterSession> &session){
	PROFILE_ME;

	const auto sessionMap = g_sessionMap.lock();
	if(!sessionMap){
		LOG_EMPERY_CENTER_WARNING("Cluster session map is not loaded.");
		DEBUG_THROW(Exception, sslit("Cluster session map is not loaded"));
	}

	const auto sessionIt = sessionMap->find<1>(session);
	if(sessionIt != sessionMap->end<1>()){
		LOG_EMPERY_CENTER_WARNING("Duplicate cluster session");
		DEBUG_THROW(Exception, sslit("Duplicate cluster session"));
	}

	auto it = sessionMap->find<0>(std::make_pair(mapX, mapY));
	if(it == sessionMap->end<0>()){
		it = sessionMap->insert<0>(it, ClusterSessionElement(mapX, mapY, session));
	} else {
		if(!it->session.expired()){
			LOG_EMPERY_CENTER_WARNING("Map session conflict: mapX = ", mapX, ", mapY = ", mapY);
			DEBUG_THROW(Exception, sslit("Map session conflict"));
		}
		sessionMap->setKey<0, 1>(it, session);
	}
}

std::pair<boost::int64_t, boost::int64_t> ClusterSessionMap::requireMapCoord(const boost::weak_ptr<ClusterSession> &session){
	PROFILE_ME;

	PROFILE_ME;

	const auto sessionMap = g_sessionMap.lock();
	if(!sessionMap){
		LOG_EMPERY_CENTER_WARNING("Cluster session map is not loaded.");
		DEBUG_THROW(Exception, sslit("Cluster session map is not loaded"));
	}

	const auto it = sessionMap->find<1>(session);
	if(it == sessionMap->end<1>()){
		LOG_EMPERY_CENTER_WARNING("Cluster session map is not found");
		DEBUG_THROW(Exception, sslit("Cluster session map is not found"));
	}
	return it->coord;
}
std::pair<boost::uint64_t, boost::uint64_t> ClusterSessionMap::requireMapSize(){
	PROFILE_ME;

	return std::make_pair(g_mapWidth, g_mapHeight);
}

}
