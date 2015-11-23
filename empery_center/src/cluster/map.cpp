#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/kc_map.hpp"
#include "../msg/ck_map.hpp"
#include "../msg/err_map.hpp"
#include "../msg/kill.hpp"
#include "../singletons/world_map.hpp"
#include "../map_object.hpp"

namespace EmperyCenter {

CLUSTER_SERVLET(Msg::KC_MapRegisterCluster, cluster, req){
	const auto center_rectangle = WorldMap::get_cluster_range(Coord(0, 0));
	const auto map_width  = static_cast<boost::uint32_t>(center_rectangle.width());
	const auto map_height = static_cast<boost::uint32_t>(center_rectangle.height());

	const auto num_coord = Coord(req.numerical_x, req.numerical_y);
	const auto inf_x = static_cast<boost::int64_t>(INT64_MAX / map_width);
	const auto inf_y = static_cast<boost::int64_t>(INT64_MAX / map_height);
	if((num_coord.x() <= -inf_x) || (inf_x <= num_coord.x()) || (num_coord.y() <= -inf_y) || (inf_y <= num_coord.y())){
		LOG_EMPERY_CENTER_WARNING("Invalid numerical coord: num_coord = ", num_coord, ", inf_x = ", inf_x, ", inf_y = ", inf_y);
		return Response(Msg::KILL_INVALID_NUMERICAL_COORD) <<num_coord;
	}
	const auto cluster_range = WorldMap::get_cluster_range(Coord(num_coord.x() * map_width, num_coord.y() * map_height));
	const auto cluster_coord = cluster_range.bottom_left();
	LOG_EMPERY_CENTER_DEBUG("Registering cluster server: num_coord = ", num_coord, ", cluster_range = ", cluster_range);

	const auto old_cluster = WorldMap::get_cluster(cluster_coord);
	if(old_cluster){
		LOG_EMPERY_CENTER_ERROR("Cluster server conflict: num_coord = ", num_coord, ", cluster_coord = ", cluster_coord);
		return Response(Msg::KILL_CLUSTER_SERVER_CONFLICT) <<cluster_coord;
	}

	Msg::CK_MapClusterRegistrationSucceeded msg;
	msg.cluster_x = cluster_range.left();
	msg.cluster_y = cluster_range.bottom();
	msg.width     = cluster_range.width();
	msg.height    = cluster_range.height();
	cluster->send(msg);

	WorldMap::set_cluster(cluster, cluster_coord);
	WorldMap::synchronize_cluster(cluster, cluster_coord);

	return Response();
}

CLUSTER_SERVLET(Msg::KC_MapUpdateMapObject, cluster, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto map_object = WorldMap::get_map_object(map_object_uuid);
	if(!map_object){
		return Response(Msg::ERR_NO_SUCH_MAP_OBJECT) <<map_object_uuid;
	}

	boost::container::flat_map<AttributeId, boost::int64_t> modifiers;
	modifiers.reserve(req.attributes.size());
	for(auto it = req.attributes.begin(); it != req.attributes.end(); ++it){
		modifiers.emplace(AttributeId(it->attribute_id), it->value);
	}
	map_object->set_attributes(modifiers);

	map_object->set_coord(Coord(req.x, req.y)); // noexcept

	return Response();
}

CLUSTER_SERVLET(Msg::KC_MapRemoveMapObject, cluster, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto map_object = WorldMap::get_map_object(map_object_uuid);
	if(!map_object){
		return Response(Msg::ERR_NO_SUCH_MAP_OBJECT) <<map_object_uuid;
	}

	map_object->delete_from_game();

	return Response();
}

}
