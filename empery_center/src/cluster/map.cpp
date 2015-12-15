#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/ks_map.hpp"
#include "../msg/sk_map.hpp"
#include "../msg/err_map.hpp"
#include "../msg/kill.hpp"
#include "../singletons/world_map.hpp"
#include "../map_object.hpp"

namespace EmperyCenter {

CLUSTER_SERVLET(Msg::KS_MapRegisterCluster, cluster, req){
	const auto center_rectangle = WorldMap::get_cluster_scope_by_coord(Coord(0, 0));
	const auto map_width  = static_cast<boost::uint32_t>(center_rectangle.width());
	const auto map_height = static_cast<boost::uint32_t>(center_rectangle.height());

	const auto num_coord = Coord(req.numerical_x, req.numerical_y);
	const auto inf_x = static_cast<boost::int64_t>(INT64_MAX / map_width);
	const auto inf_y = static_cast<boost::int64_t>(INT64_MAX / map_height);
	if((num_coord.x() <= -inf_x) || (inf_x <= num_coord.x()) || (num_coord.y() <= -inf_y) || (inf_y <= num_coord.y())){
		LOG_EMPERY_CENTER_WARNING("Invalid numerical coord: num_coord = ", num_coord, ", inf_x = ", inf_x, ", inf_y = ", inf_y);
		return Response(Msg::KILL_INVALID_NUMERICAL_COORD) <<num_coord;
	}
	const auto cluster_scope = WorldMap::get_cluster_scope_by_coord(Coord(num_coord.x() * map_width, num_coord.y() * map_height));
	const auto cluster_coord = cluster_scope.bottom_left();
	LOG_EMPERY_CENTER_DEBUG("Registering cluster server: num_coord = ", num_coord, ", cluster_scope = ", cluster_scope);

	const auto old_cluster = WorldMap::get_cluster(cluster_coord);
	if(old_cluster){
		LOG_EMPERY_CENTER_ERROR("Cluster server conflict: num_coord = ", num_coord, ", cluster_coord = ", cluster_coord);
		return Response(Msg::KILL_CLUSTER_SERVER_CONFLICT) <<cluster_coord;
	}
	const auto old_scope = WorldMap::get_cluster_scope(cluster);
	if((old_scope.width() != 0) || (old_scope.height() != 0)){
		LOG_EMPERY_CENTER_ERROR("Cluster server already registered: num_coord = ", num_coord, ", cluster_coord = ", cluster_coord);
		return Response(Msg::KILL_MAP_SERVER_ALREADY_REGISTERED);
	}

	Msg::SK_MapClusterRegistrationSucceeded msg;
	msg.cluster_x = cluster_scope.left();
	msg.cluster_y = cluster_scope.bottom();
	msg.width     = cluster_scope.width();
	msg.height    = cluster_scope.height();
	cluster->send(msg);

	WorldMap::set_cluster(cluster, cluster_coord);

	WorldMap::synchronize_cluster(cluster, cluster_scope);

	return Response();
}

CLUSTER_SERVLET(Msg::KS_MapUpdateMapObject, cluster, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto map_object = WorldMap::get_map_object(map_object_uuid);
	if(!map_object){
		return Response(Msg::ERR_NO_SUCH_MAP_OBJECT) <<map_object_uuid;
	}
	const auto test_cluster = WorldMap::get_cluster(map_object->get_coord());
	if(cluster != test_cluster){
		return Response(Msg::ERR_MAP_OBJECT_ON_ANOTHER_CLUSTER);
	}


	boost::container::flat_map<AttributeId, boost::int64_t> modifiers;
	modifiers.reserve(req.attributes.size());
	for(auto it = req.attributes.begin(); it != req.attributes.end(); ++it){
		modifiers.emplace(AttributeId(it->attribute_id), it->value);
	}
	map_object->set_attributes(modifiers);

	const auto old_coord = map_object->get_coord();
	const auto new_coord = Coord(req.x, req.y);
	map_object->set_coord(new_coord); // noexcept

	const auto new_cluster = WorldMap::get_cluster(new_coord);
	if(!new_cluster){
		LOG_EMPERY_CENTER_DEBUG("No cluster there: new_coord = ", new_coord);
		// 注意，这个不能和上面那个 set_coord() 合并成一个操作。
		// 如果我们跨服务器设定了坐标，在这里地图服务器会重新同步数据，并删除旧的路径。
		map_object->set_coord(old_coord); // noexcept
	}

	return Response();
}

CLUSTER_SERVLET(Msg::KS_MapRemoveMapObject, cluster, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto map_object = WorldMap::get_map_object(map_object_uuid);
	if(!map_object){
		return Response(Msg::ERR_NO_SUCH_MAP_OBJECT) <<map_object_uuid;
	}
	const auto test_cluster = WorldMap::get_cluster(map_object->get_coord());
	if(cluster != test_cluster){
		return Response(Msg::ERR_MAP_OBJECT_ON_ANOTHER_CLUSTER);
	}

	map_object->delete_from_game();

	return Response();
}

}
