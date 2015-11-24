#include "../precompiled.hpp"
#include "common.hpp"
#include "../../../empery_center/src/msg/ck_map.hpp"
#include "../../../empery_center/src/msg/err_map.hpp"
#include "../singletons/world_map.hpp"
#include "../map_object.hpp"
#include "../checked_arithmetic.hpp"

namespace EmperyCluster {

CLUSTER_SERVLET(Msg::CK_MapClusterRegistrationSucceeded, cluster, req){
	const auto scope = Rectangle(req.cluster_x, req.cluster_y, req.width, req.height);
	LOG_EMPERY_CLUSTER_INFO("Cluster server registered successfully: scope = ", scope);

	WorldMap::set_cluster(cluster, scope);

	return Response();
}

CLUSTER_SERVLET(Msg::CK_MapAddMapObject, cluster, req){
	const auto map_object_uuid    = MapObjectUuid(req.map_object_uuid);
	const auto map_object_type_id = MapObjectTypeId(req.map_object_type_id);
	const auto owner_uuid         = AccountUuid(req.owner_uuid);
	const auto coord              = Coord(req.x, req.y);

	boost::container::flat_map<AttributeId, boost::int64_t> attributes;
	attributes.reserve(req.attributes.size());
	for(auto it = req.attributes.begin(); it != req.attributes.end(); ++it){
		attributes.emplace(AttributeId(it->attribute_id), it->value);
	}

	LOG_EMPERY_CLUSTER_DEBUG("Creating map object: map_object_uuid = ", map_object_uuid,
		", map_object_type_id = ", map_object_type_id, ", owner_uuid = ", owner_uuid, ", coord = ", coord);
	const auto map_object = boost::make_shared<MapObject>(map_object_uuid, map_object_type_id, owner_uuid, coord, std::move(attributes));
	WorldMap::replace_map_object(map_object);

	return Response();
}

CLUSTER_SERVLET(Msg::CK_MapRemoveMapObject, cluster, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);

	LOG_EMPERY_CLUSTER_DEBUG("Removing map object: map_object_uuid = ", map_object_uuid);
	WorldMap::remove_map_object(map_object_uuid);

	return Response();
}

CLUSTER_SERVLET(Msg::CK_MapSetWaypoints, cluster, req){
	const auto map_object_uuid    = MapObjectUuid(req.map_object_uuid);
	const auto attack_target_uuid = MapObjectUuid(req.attack_target_uuid);

	const auto map_object = WorldMap::get_map_object(map_object_uuid);
	if(!map_object){
		return Response(Msg::ERR_NO_SUCH_MAP_OBJECT) <<map_object_uuid;
	}

	const auto now = Poseidon::get_fast_mono_clock();

	std::deque<MapObject::Waypoint> waypoints;
	auto pt = MapObject::Waypoint(now, Coord(req.x, req.y));
	waypoints.emplace_back(pt);
	for(auto it = req.waypoints.begin(); it != req.waypoints.end(); ++it){
		pt = MapObject::Waypoint(saturated_add(pt.timestamp, it->delay), Coord(pt.coord.x() + it->dx, pt.coord.y() + it->dy));
		waypoints.emplace_back(pt);
	}
	map_object->set_waypoints(std::move(waypoints));

	map_object->set_attack_target_uuid(attack_target_uuid);

	return Response();
}

}
