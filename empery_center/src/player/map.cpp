#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/cs_map.hpp"
#include "../msg/sc_map.hpp"
#include "../msg/cerr_map.hpp"
#include "../singletons/cluster_session_map.hpp"
#include "../singletons/map_object_map.hpp"
#include "../map_object.hpp"

namespace EmperyCenter {

PLAYER_SERVLET(Msg::CS_MapQueryWorldMap, accountUuid, session, req){
	(void)req;

	ClusterSessionMap::SessionContainer clusters;
	ClusterSessionMap::getAll(clusters);
	const auto mapSize = ClusterSessionMap::requireMapSize();

	Msg::SC_MapWorldMapList msg;
	msg.maps.reserve(clusters.size());
	for(auto it = clusters.begin(); it != clusters.end(); ++it){
		msg.maps.emplace_back();
		auto &map = msg.maps.back();
		map.mapX = it->first.first;
		map.mapX = it->first.second;
	}
	msg.mapWidth = mapSize.first;
	msg.mapHeight = mapSize.second;
	session->send(msg);

	return Response();
}

PLAYER_SERVLET(Msg::CS_MapSetView, accountUuid, session, req){
	MapObjectMap::setPlayerView(session, Rectangle(req.x, req.y, req.width, req.height));

	return Response();
}

PLAYER_SERVLET(Msg::CS_MapRefreshView, accountUuid, session, req){
	MapObjectMap::synchronizePlayerView(session, Rectangle(req.x, req.y, req.width, req.height));

	return Response();
}

PLAYER_SERVLET(Msg::CS_MapSetObjectPath, accountUuid, session, req){
	const auto mapObjectUuid = MapObjectUuid(req.mapObjectUuid);
	const auto mapObject = MapObjectMap::get(mapObjectUuid);
	if(!mapObject){
		return { Msg::CERR_NO_SUCH_OBJECT, mapObjectUuid.str() };
	}
	if(mapObject->getOwnerUuid() != accountUuid){
		return { Msg::CERR_NOT_YOUR_OBJECT, mapObject->getOwnerUuid().str() };
	}
	const auto fromCoord = mapObject->getCoord();
	if((fromCoord.x() != req.x) || (fromCoord.y() != req.y)){
		return { Msg::CERR_OBJECT_COORD_MISMATCH, boost::lexical_cast<std::string>(fromCoord) };
	}
	// TODO 判断能不能走。

	return Response();
}

}
