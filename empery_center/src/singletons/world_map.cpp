#include "../precompiled.hpp"
#include "world_map.hpp"
#include <poseidon/multi_index_map.hpp>
#include "../cluster_session.hpp"

namespace EmperyCenter {

namespace {
	boost::uint64_t g_mapWidth  = 400;
	boost::uint64_t g_mapHeight = 300;

	struct WorldMapElement {
		std::pair<boost::int64_t, boost::int64_t> coord;
		boost::weak_ptr<ClusterSession> session;

		WorldMapElement(boost::int64_t mapX_, boost::int64_t mapY_, boost::weak_ptr<ClusterSession> session_)
			: coord(mapX_, mapY_), session(std::move(session_))
		{
		}
	};

	MULTI_INDEX_MAP(WorldMapDelegator, WorldMapElement,
		UNIQUE_MEMBER_INDEX(coord)
		UNIQUE_MEMBER_INDEX(session)
	)

	boost::weak_ptr<WorldMapDelegator> g_worldMap;

	MODULE_RAII(handles){
		getConfig(g_mapWidth,  "map_width");
		getConfig(g_mapHeight, "map_height");
		LOG_EMPERY_CENTER_DEBUG("> Map width = ", g_mapWidth, ", map height = ", g_mapHeight);

		const auto worldMap = boost::make_shared<WorldMapDelegator>();
		g_worldMap = worldMap;
		handles.push(worldMap);
	}
}

boost::shared_ptr<ClusterSession> WorldMap::get(boost::int64_t mapX, boost::int64_t mapY){
	PROFILE_ME;

	const auto worldMap = g_worldMap.lock();
	if(!worldMap){
		LOG_EMPERY_CENTER_DEBUG("World map is not loaded.");
		return { };
	}

	const auto it = worldMap->find<0>(std::make_pair(mapX, mapY));
	if(it == worldMap->end<0>()){
		LOG_EMPERY_CENTER_DEBUG("World map is not found: mapX = ", mapX, ", mapY = ", mapY);
		return { };
	}
	auto session = it->session.lock();
	if(!session){
		LOG_EMPERY_CENTER_DEBUG("Session expired: mapX = ", mapX, ", mapY = ", mapY);
		worldMap->erase<0>(it);
		return { };
	}
	return session;
}
void WorldMap::getAll(boost::container::flat_map<std::pair<boost::int64_t, boost::int64_t>, boost::shared_ptr<ClusterSession>> &ret){
	PROFILE_ME;

	const auto worldMap = g_worldMap.lock();
	if(!worldMap){
		LOG_EMPERY_CENTER_DEBUG("World map is not loaded.");
		return;
	}

	ret.reserve(ret.size() + worldMap->size());
	for(auto it = worldMap->begin(); it != worldMap->end(); ++it){
		auto session = it->session.lock();
		if(!session){
			continue;
		}
		ret.emplace(it->coord, std::move(session));
	}
}
void WorldMap::set(boost::int64_t mapX, boost::int64_t mapY, const boost::shared_ptr<ClusterSession> &session){
	PROFILE_ME;

	const auto worldMap = g_worldMap.lock();
	if(!worldMap){
		LOG_EMPERY_CENTER_WARNING("World map is not loaded.");
		DEBUG_THROW(Exception, sslit("World map is not loaded"));
	}

	const auto sessionIt = worldMap->find<1>(session);
	if(sessionIt != worldMap->end<1>()){
		LOG_EMPERY_CENTER_WARNING("Duplicate cluster session");
		DEBUG_THROW(Exception, sslit("Duplicate cluster session"));
	}

	auto it = worldMap->find<0>(std::make_pair(mapX, mapY));
	if(it == worldMap->end<0>()){
		it = worldMap->insert<0>(it, WorldMapElement(mapX, mapY, session));
	} else {
		if(!it->session.expired()){
			LOG_EMPERY_CENTER_WARNING("Map session conflict: mapX = ", mapX, ", mapY = ", mapY);
			DEBUG_THROW(Exception, sslit("Map session conflict"));
		}
		worldMap->setKey<0, 1>(it, session);
	}
}

std::pair<boost::int64_t, boost::int64_t> WorldMap::requireMapCoord(const boost::weak_ptr<ClusterSession> &session){
	PROFILE_ME;

	PROFILE_ME;

	const auto worldMap = g_worldMap.lock();
	if(!worldMap){
		LOG_EMPERY_CENTER_WARNING("World map is not loaded.");
		DEBUG_THROW(Exception, sslit("World map is not loaded"));
	}

	const auto it = worldMap->find<1>(session);
	if(it == worldMap->end<1>()){
		LOG_EMPERY_CENTER_WARNING("World map is not found");
		DEBUG_THROW(Exception, sslit("World map is not found"));
	}
	return it->coord;
}
std::pair<boost::uint64_t, boost::uint64_t> WorldMap::requireMapSize(){
	PROFILE_ME;

	return std::make_pair(g_mapWidth, g_mapHeight);
}

}
