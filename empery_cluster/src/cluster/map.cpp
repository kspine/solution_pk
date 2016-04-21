#include "../precompiled.hpp"
#include "common.hpp"
#include "../../../empery_center/src/msg/sk_map.hpp"
#include "../../../empery_center/src/msg/err_map.hpp"
#include "../singletons/world_map.hpp"
#include "../map_cell.hpp"
#include "../map_object.hpp"
#include "../resource_crate.hpp"

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
	const auto map_object_type_id = MapObjectTypeId(req.map_object_type_id);
	const auto owner_uuid         = AccountUuid(req.owner_uuid);
	const auto parent_object_uuid = MapObjectUuid(req.parent_object_uuid);
	const bool garrisoned         = req.garrisoned;
	const auto coord              = Coord(req.x, req.y);

	boost::shared_ptr<MapObject> map_object;
	{
		auto old_map_object = WorldMap::get_map_object(map_object_uuid);
		if(old_map_object){
			const auto old_cluster = WorldMap::get_cluster(old_map_object->get_coord());
			if(old_cluster == cluster){
				// 替换旧的。
				map_object = std::move(old_map_object);
			}
		}
	}

	boost::container::flat_map<AttributeId, std::int64_t> attributes;
	attributes.reserve(req.attributes.size());
	for(auto it = req.attributes.begin(); it != req.attributes.end(); ++it){
		attributes.emplace(AttributeId(it->attribute_id), it->value);
	}
	if(!map_object || (map_object->is_garrisoned() != garrisoned)){
		LOG_EMPERY_CLUSTER_TRACE("Replacing map object: map_object_uuid = ", map_object_uuid,
			", map_object_type_id = ", map_object_type_id, ", owner_uuid = ", owner_uuid, ", garrisoned = ", garrisoned, ", coord = ", coord);
		map_object = boost::make_shared<MapObject>(map_object_uuid,
			map_object_type_id, owner_uuid, parent_object_uuid, garrisoned, cluster, coord, std::move(attributes));
		WorldMap::replace_map_object_no_synchronize(cluster, map_object);
	} else {
		LOG_EMPERY_CLUSTER_TRACE("Updating map object: map_object_uuid = ", map_object_uuid,
			", map_object_type_id = ", map_object_type_id, ", owner_uuid = ", owner_uuid, ", garrisoned = ", garrisoned, ", coord = ", coord);
		map_object->set_attributes_no_synchronize(std::move(attributes));
	}

	return Response();
}

CLUSTER_SERVLET(Msg::SK_MapRemoveMapObject, cluster, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);

	LOG_EMPERY_CLUSTER_TRACE("Removing map object: map_object_uuid = ", map_object_uuid);
	WorldMap::remove_map_object_no_synchronize(cluster, map_object_uuid);

	return Response();
}

CLUSTER_SERVLET(Msg::SK_MapSetAction, cluster, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);

	const auto map_object = WorldMap::get_map_object(map_object_uuid);
	if(!map_object){
		return Response(Msg::ERR_NO_SUCH_MAP_OBJECT) <<map_object_uuid;
	}

	const auto from_coord = Coord(req.x, req.y);
	std::deque<std::pair<signed char, signed char>> waypoints;
	for(auto it = req.waypoints.begin(); it != req.waypoints.end(); ++it){
		waypoints.emplace_back(it->dx, it->dy);
	}
	map_object->set_action(from_coord, std::move(waypoints), static_cast<MapObject::Action>(req.action), std::move(req.param));

	return Response();
}

CLUSTER_SERVLET(Msg::SK_MapCreateResourceCrate, cluster, req){
	const auto resource_crate_uuid  = ResourceCrateUuid(req.resource_crate_uuid);
	const auto resource_id          = ResourceId(req.resource_id);
	const auto amount_max           = req.amount_max;
	const auto coord                = Coord(req.x, req.y);
	const auto created_time         = req.created_time;
	const auto expiry_time          = req.expiry_time;
	const auto amount_remaining     = req.amount_remaining;

	boost::shared_ptr<ResourceCrate> resource_crate;
	{
		auto old_resource_crate = WorldMap::get_resource_crate(resource_crate_uuid);
		if(old_resource_crate){
			const auto old_cluster = WorldMap::get_cluster(old_resource_crate->get_coord());
			if(old_cluster == cluster){
				// 替换旧的。
				resource_crate = std::move(old_resource_crate);
			}
		}
	}

	if(!resource_crate){
		LOG_EMPERY_CLUSTER_TRACE("Replacing resource crate: resource_crate_uuid = ", resource_crate_uuid,
			", resource_id = ", resource_id, ", amount_max = ", amount_max, ", created_time = ", created_time,", expiry_time = ", expiry_time,", amount_remaining = ", amount_remaining, ", coord = ", coord);
		resource_crate = boost::make_shared<ResourceCrate>(resource_crate_uuid,
			resource_id, amount_max, created_time, expiry_time,amount_remaining, cluster, coord);
		WorldMap::replace_resource_crate_no_synchronize(cluster, resource_crate);
	} else {
		LOG_EMPERY_CLUSTER_TRACE("Updating resource crate: resource_crate_uuid = ", resource_crate_uuid,
			", resource_id = ", resource_id, ", amount_max = ", amount_max, ", created_time = ", created_time,", expiry_time = ", expiry_time,", amount_remaining = ", amount_remaining, ", coord = ", coord);
		resource_crate->set_amount_remaing(amount_remaining);
	}

	return Response();
}

CLUSTER_SERVLET(Msg::SK_MapRemoveResourceCrate, cluster, req){
	const auto resource_crate_uuid  = ResourceCrateUuid(req.resource_crate_uuid);

	LOG_EMPERY_CLUSTER_TRACE("Removing resource crate: resource_crate_uuid = ", resource_crate_uuid);
	WorldMap::remove_resource_crate_no_synchronize(cluster, resource_crate_uuid);

	return Response();
}

}
