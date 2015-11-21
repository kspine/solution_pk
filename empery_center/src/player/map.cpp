#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/cs_map.hpp"
#include "../msg/sc_map.hpp"
#include "../msg/err_map.hpp"
#include "../singletons/world_map.hpp"
#include "../map_object.hpp"
#include "../singletons/cluster_session_map.hpp"
#include "../cluster_session.hpp"

namespace EmperyCenter {

PLAYER_SERVLET(Msg::CS_MapQueryWorldMap, account_uuid, session, /* req */){
	boost::container::flat_map<Coord, boost::shared_ptr<ClusterSession>> servers;
	ClusterSessionMap::get_all(servers);
	const auto center_rectangle = ClusterSessionMap::get_server_map_range(Coord(0, 0));

	Msg::SC_MapWorldMapList msg;
	msg.servers.reserve(servers.size());
	for(auto it = servers.begin(); it != servers.end(); ++it){
		msg.servers.emplace_back();
		auto &server = msg.servers.back();
		server.server_x = it->first.x();
		server.server_x = it->first.y();
	}
	msg.map_width  = center_rectangle.width();
	msg.map_height = center_rectangle.height();
	session->send(msg);

	return Response();
}

PLAYER_SERVLET(Msg::CS_MapSetView, account_uuid, session, req){
	WorldMap::set_player_view(session, Rectangle(req.x, req.y, req.width, req.height));

	return Response();
}

PLAYER_SERVLET(Msg::CS_MapRefreshView, account_uuid, session, req){
	WorldMap::synchronize_player_view(session, Rectangle(req.x, req.y, req.width, req.height));

	return Response();
}

PLAYER_SERVLET(Msg::CS_MapSetWaypoints, account_uuid, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto map_object = WorldMap::get_map_object(map_object_uuid);
	if(!map_object){
		return Response(Msg::ERR_NO_SUCH_OBJECT) <<map_object_uuid;
	}
	if(map_object->get_owner_uuid() != account_uuid){
		return Response(Msg::ERR_NOT_YOUR_OBJECT) <<map_object->get_owner_uuid();
	}
	const auto from_coord = map_object->get_coord();
	if((from_coord.x() != req.x) || (from_coord.y() != req.y)){
		return Response(Msg::ERR_OBJECT_COORD_MISMATCH) <<from_coord;
	}
	// TODO 判断能不能走。

	std::deque<Coord> abs_coords;
	abs_coords.emplace_front(from_coord);
	for(std::size_t i = 0; i < req.waypoints.size(); ++i){
		const auto &rel_coord = req.waypoints.at(i);
		if((rel_coord.dx < -1) || (rel_coord.dx > 1) || (rel_coord.dy < -1) || (rel_coord.dy > 1)){
			LOG_EMPERY_CENTER_DEBUG("Invalid relative coord: i = ", i, ", dx = ", rel_coord.dx, ", dy = ", rel_coord.dy);
			return Response(Msg::ERR_BROKEN_PATH) <<i;
		}
		abs_coords.emplace_back(abs_coords.back().x() + rel_coord.dx, abs_coords.back().y() + rel_coord.dy);
	}
	const auto server_coord = ClusterSessionMap::get_server_coord_from_map_coord(from_coord);
	const auto cluster = ClusterSessionMap::get(server_coord);
	if(!cluster){
		LOG_EMPERY_CENTER_WARNING("Lost connection to map server: server_coord = ", server_coord);
		return Response(Msg::ERR_MAP_SERVER_CONNECTION_LOST) <<server_coord;
	}
/*	cluster->
const auto cluster = 
static boost::shared_ptr<ClusterSession> get(Coord server_coord);
static void get_all(boost::container::flat_map<Coord, boost::shared_ptr<ClusterSession>> &ret);
static void set(Coord server_coord, const boost::shared_ptr<ClusterSession> &session);

static Coord get_server_coord(const boost::weak_ptr<ClusterSession> &session);
static Coord require_server_coord(const boost::weak_ptr<ClusterSession> &session);

static Coord get_server_coord_from_map_coord(Coord coord);
static Rectangle get_server_map_range(Coord server_coord);
*/	// TODO 设定路径。

	return Response();
}

}
