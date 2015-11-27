#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/cs_map.hpp"
#include "../msg/sc_map.hpp"
#include "../msg/err_map.hpp"
#include "../singletons/world_map.hpp"
#include "../utilities.hpp"
#include "../map_object.hpp"
#include "../cluster_session.hpp"
#include "../msg/ck_map.hpp"
#include "../data/map_object_type.hpp"

namespace EmperyCenter {

PLAYER_SERVLET(Msg::CS_MapQueryWorldMap, account_uuid, session, /* req */){
	std::vector<std::pair<Rectangle, boost::shared_ptr<ClusterSession>>> clusters;
	WorldMap::get_all_clusters(clusters);
	const auto center_rectangle = WorldMap::get_cluster_scope_by_coord(Coord(0, 0));

	Msg::SC_MapWorldMapList msg;
	msg.clusters.reserve(clusters.size());
	for(auto it = clusters.begin(); it != clusters.end(); ++it){
		msg.clusters.emplace_back();
		auto &cluster = msg.clusters.back();
		cluster.x = it->first.left();
		cluster.y = it->first.bottom();
	}
	msg.cluster_width  = center_rectangle.width();
	msg.cluster_height = center_rectangle.height();
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
		return Response(Msg::ERR_NO_SUCH_MAP_OBJECT) <<map_object_uuid;
	}
	if(map_object->get_owner_uuid() != account_uuid){
		return Response(Msg::ERR_NOT_YOUR_MAP_OBJECT) <<map_object->get_owner_uuid();
	}
	const auto map_object_type_data = Data::MapObjectType::require(map_object->get_map_object_type_id());
	const auto ms_per_cell = 1000u; // FIXME   map_object_type_data->ms_per_cell;
	if(ms_per_cell == 0){
		return Response(Msg::ERR_NOT_MOVABLE_MAP_OBJECT) <<map_object_type_data->map_object_type_id;
	}

	const auto attack_target_uuid = MapObjectUuid(req.attack_target_uuid);

	const auto from_coord = map_object->get_coord();
	const auto cluster = WorldMap::get_cluster(from_coord);
	if(!cluster){
		LOG_EMPERY_CENTER_WARNING("No cluster server available: from_coord = ", from_coord);
		return Response(Msg::ERR_CLUSTER_CONNECTION_LOST) <<from_coord;
	}

	Msg::CK_MapSetWaypoints kreq;
	kreq.map_object_uuid = map_object->get_map_object_uuid().str();
	kreq.x = from_coord.x();
	kreq.y = from_coord.y();
	kreq.waypoints.reserve(req.waypoints.size());
	auto last_coord = from_coord;
	for(std::size_t i = 0; i < req.waypoints.size(); ++i){
		const auto &step = req.waypoints.at(i);
		if((step.dx < -1) || (step.dx > 1) || (step.dy < -1) || (step.dy > 1)){
			LOG_EMPERY_CENTER_DEBUG("Invalid relative coord: i = ", i, ", dx = ", step.dx, ", dy = ", step.dy);
			return Response(Msg::ERR_BROKEN_PATH) <<i;
		}
		const auto next_coord = Coord(last_coord.x() + step.dx, last_coord.y() + step.dy);
		if(get_distance_of_coords(last_coord, next_coord) > 1){
			LOG_EMPERY_CENTER_DEBUG("Broken path: last_coord = ", last_coord, ", next_coord = ", next_coord);
			return Response(Msg::ERR_BROKEN_PATH) <<i;
		}
		last_coord = next_coord;

		kreq.waypoints.emplace_back();
		auto &waypoint = kreq.waypoints.back();
		waypoint.delay = ms_per_cell;
		waypoint.dx    = step.dx;
		waypoint.dy    = step.dy;
	}
	kreq.attack_target_uuid = attack_target_uuid.str();
	auto kresult = cluster->send_and_wait(kreq);
	if(kresult.first != Msg::ST_OK){
		LOG_EMPERY_CENTER_DEBUG("Cluster server returned an error: code = ", kresult.first, ", msg = ", kresult.second);
		return std::move(kresult);
	}

	return Response();
}

}
