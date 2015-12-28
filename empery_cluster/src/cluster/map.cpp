#include "../precompiled.hpp"
#include "common.hpp"
#include "../../../empery_center/src/msg/sk_map.hpp"
#include "../../../empery_center/src/msg/err_map.hpp"
#include "../singletons/world_map.hpp"
#include "../map_cell.hpp"
#include "../overlay.hpp"
#include "../map_object.hpp"

namespace EmperyCluster {

CLUSTER_SERVLET(Msg::SK_MapClusterRegistrationSucceeded, cluster, req){
	const auto cluster_coord = Coord(req.cluster_x, req.cluster_y);
	LOG_EMPERY_CLUSTER_INFO("Cluster server registered successfully: cluster_coord = ", cluster_coord);

	WorldMap::set_cluster(cluster, cluster_coord);

	return Response();
}

CLUSTER_SERVLET(Msg::SK_MapAddMapCell, cluster, req){
	auto coord              = Coord(req.x, req.y);
	auto parent_object_uuid = MapObjectUuid(req.parent_object_uuid);
	auto owner_uuid         = AccountUuid(req.owner_uuid);

	boost::container::flat_map<AttributeId, std::int64_t> attributes;
	attributes.reserve(req.attributes.size());
	for(auto it = req.attributes.begin(); it != req.attributes.end(); ++it){
		attributes.emplace(AttributeId(it->attribute_id), it->value);
	}

	LOG_EMPERY_CLUSTER_TRACE("Creating map cell: coord = ", coord,
		", parent_object_uuid = ", parent_object_uuid, ", owner_uuid = ", owner_uuid);
	const auto map_cell = boost::make_shared<MapCell>(coord, parent_object_uuid, owner_uuid, std::move(attributes));
	WorldMap::replace_map_cell_no_synchronize(cluster, map_cell);

	return Response();
}

CLUSTER_SERVLET(Msg::SK_MapAddMapObject, cluster, req){
	const auto map_object_uuid    = MapObjectUuid(req.map_object_uuid);

	auto map_object = WorldMap::get_map_object(map_object_uuid);
	if(map_object){
		const auto old_cluster = WorldMap::get_cluster(map_object->get_coord());
		if(old_cluster != cluster){
			// 替换旧的。
			map_object.reset();
		}
	}
	if(!map_object){
		const auto map_object_type_id = MapObjectTypeId(req.map_object_type_id);
		const auto owner_uuid         = AccountUuid(req.owner_uuid);
		const auto coord              = Coord(req.x, req.y);

		boost::container::flat_map<AttributeId, std::int64_t> attributes;
		attributes.reserve(req.attributes.size());
		for(auto it = req.attributes.begin(); it != req.attributes.end(); ++it){
			attributes.emplace(AttributeId(it->attribute_id), it->value);
		}

		LOG_EMPERY_CLUSTER_TRACE("Creating map object: map_object_uuid = ", map_object_uuid,
			", map_object_type_id = ", map_object_type_id, ", owner_uuid = ", owner_uuid, ", coord = ", coord);
		map_object = boost::make_shared<MapObject>(map_object_uuid,
			map_object_type_id, owner_uuid, cluster, coord, std::move(attributes));
		WorldMap::replace_map_object_no_synchronize(cluster, map_object);
	}

	return Response();
}

CLUSTER_SERVLET(Msg::SK_MapRemoveMapObject, cluster, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);

	LOG_EMPERY_CLUSTER_DEBUG("Removing map object: map_object_uuid = ", map_object_uuid);
	WorldMap::remove_map_object_no_synchronize(cluster, map_object_uuid);

	return Response();
}

CLUSTER_SERVLET(Msg::SK_MapSetAction, cluster, req){
	const auto map_object_uuid    = MapObjectUuid(req.map_object_uuid);

	const auto map_object = WorldMap::get_map_object(map_object_uuid);
	if(!map_object){
		return Response(Msg::ERR_NO_SUCH_MAP_OBJECT) <<map_object_uuid;
	}

	const auto from_coord = Coord(req.x, req.y);
	std::deque<MapObject::Waypoint> waypoints;
	for(auto it = req.waypoints.begin(); it != req.waypoints.end(); ++it){
		waypoints.emplace_back(it->delay, it->dx, it->dy);
	}
	map_object->set_action(from_coord, std::move(waypoints), static_cast<MapObject::Action>(req.action), std::move(req.param));

	return Response();
}

CLUSTER_SERVLET(Msg::SK_MapAddOverlay, cluster, req){
	auto cluster_coord      = Coord(req.cluster_x, req.cluster_y);
	auto overlay_group_name = std::move(req.overlay_group_name);
	auto overlay_id         = OverlayId(req.overlay_id);
	auto resource_id        = ResourceId(req.resource_id);
	auto resource_amount    = req.resource_amount;

	LOG_EMPERY_CLUSTER_TRACE("Creating map overlay: cluster_coord = ", cluster_coord, ", overlay_group_name = ", overlay_group_name,
		", overlay_id = ", overlay_id, ", resource_id = ", resource_id, ", resource_amount = ", resource_amount);
	const auto overlay = boost::make_shared<Overlay>(
		cluster_coord, std::move(overlay_group_name), overlay_id, resource_id, resource_amount);
	WorldMap::replace_overlay_no_synchronize(cluster, overlay);

	return Response();
}

}
