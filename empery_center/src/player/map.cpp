#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/cs_map.hpp"
#include "../msg/sc_map.hpp"
#include "../msg/cerr_map.hpp"
#include "../singletons/cluster_session_map.hpp"

namespace EmperyCenter {

PLAYER_SERVLET(Msg::CS_MapQueryWorldMap, accountUuid, session, req){
	ClusterSessionMap::SessionContainer maps;
	ClusterSessionMap::getAll(maps);
	const auto mapSize = ClusterSessionMap::requireMapSize();

	Msg::SC_MapWorldMapList msg;
	msg.maps.reserve(maps.size());
	for(auto it = maps.begin(); it != maps.end(); ++it){
		msg.maps.emplace_back();
		auto &map = msg.maps.back();
		map.mapX = it->first.first;
		map.mapX = it->first.second;
	}
	msg.mapWidth = mapSize.first;
	msg.mapHeight = mapSize.second;
	session->send(msg);
	return { };
}

}
