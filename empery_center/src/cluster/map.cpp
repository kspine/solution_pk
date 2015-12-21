#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/ks_map.hpp"
#include "../msg/sk_map.hpp"
#include "../msg/err_map.hpp"
#include "../msg/kill.hpp"
#include "../singletons/world_map.hpp"
#include "../map_object.hpp"
#include "../map_object_type_ids.hpp"
#include "../utilities.hpp"
#include "../data/map.hpp"
#include "../castle.hpp"
#include "../overlay.hpp"
#include "../data/map_object_type.hpp"

namespace EmperyCenter {

CLUSTER_SERVLET(Msg::KS_MapRegisterCluster, cluster, req){
	const auto center_rectangle = WorldMap::get_cluster_scope(Coord(0, 0));
	const auto map_width  = static_cast<boost::uint32_t>(center_rectangle.width());
	const auto map_height = static_cast<boost::uint32_t>(center_rectangle.height());

	const auto num_coord = Coord(req.numerical_x, req.numerical_y);
	const auto inf_x = static_cast<boost::int64_t>(INT64_MAX / map_width);
	const auto inf_y = static_cast<boost::int64_t>(INT64_MAX / map_height);
	if((num_coord.x() <= -inf_x) || (inf_x <= num_coord.x()) || (num_coord.y() <= -inf_y) || (inf_y <= num_coord.y())){
		LOG_EMPERY_CENTER_WARNING("Invalid numerical coord: num_coord = ", num_coord, ", inf_x = ", inf_x, ", inf_y = ", inf_y);
		return Response(Msg::KILL_INVALID_NUMERICAL_COORD) <<num_coord;
	}
	const auto cluster_scope = WorldMap::get_cluster_scope(Coord(num_coord.x() * map_width, num_coord.y() * map_height));
	const auto cluster_coord = cluster_scope.bottom_left();
	LOG_EMPERY_CENTER_DEBUG("Registering cluster server: num_coord = ", num_coord, ", cluster_scope = ", cluster_scope);

	WorldMap::set_cluster(cluster, cluster_coord);

	Msg::SK_MapClusterRegistrationSucceeded msg;
	msg.cluster_x = cluster_coord.x();
	msg.cluster_y = cluster_coord.y();
	cluster->send(msg);

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

CLUSTER_SERVLET(Msg::KS_MapHarvestOverlay, cluster, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto map_object = WorldMap::get_map_object(map_object_uuid);
	if(!map_object){
		return Response(Msg::ERR_NO_SUCH_MAP_OBJECT) <<map_object_uuid;
	}
	const auto test_cluster = WorldMap::get_cluster(map_object->get_coord());
	if(cluster != test_cluster){
		return Response(Msg::ERR_MAP_OBJECT_ON_ANOTHER_CLUSTER);
	}

	const auto parent_object_uuid = map_object->get_parent_object_uuid();
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(parent_object_uuid));
	if(!castle){
		return Response(Msg::ERR_MAP_OBJECT_PARENT_GONE) <<parent_object_uuid;
	}

	const auto coord = map_object->get_coord();

	std::vector<boost::shared_ptr<Overlay>> overlays;
	WorldMap::get_overlays_by_rectangle(overlays, Rectangle(coord, 1, 1));
	if(overlays.empty()){
		return Response(Msg::ERR_OVERLAY_ALREADY_REMOVED) <<coord;
	}
	const auto &overlay = overlays.front();
	const auto resource_amount = overlay->get_resource_amount();
	if(resource_amount == 0){
		return Response(Msg::ERR_OVERLAY_ALREADY_REMOVED) <<coord;
	}
	const auto resource_id = overlay->get_resource_id();

	boost::uint64_t capacity_remaining = 0;
	const auto max_amounts = castle->get_max_resource_amounts();
	const auto rit = max_amounts.find(resource_id);
	if(rit == max_amounts.end()){
		LOG_EMPERY_CENTER_DEBUG("There is no warehouse? map_object_uuid = ", map_object_uuid, ", resource_id = ", resource_id);
	} else {
		const auto max_amount = rit->second;
		const auto current_amount = castle->get_resource(resource_id).amount;
		capacity_remaining = saturated_sub(max_amount, current_amount);
	}

	const auto map_object_type_id = map_object->get_map_object_type_id();
	const auto map_object_type_data = Data::MapObjectType::require(map_object_type_id);
	const auto harvest_speed = map_object_type_data->harvest_speed;
	if(harvest_speed <= 0){
		return Response(Msg::ERR_ZERO_HARVEST_SPEED) <<map_object_type_id;
	}
	const auto harvest_amount = harvest_speed * req.interval / 60000.0 + map_object->get_harvest_remainder();
	const auto rounded_amount = static_cast<boost::uint64_t>(harvest_amount);
	const auto remainder = std::fdim(harvest_amount, rounded_amount);

	const auto harvested_amount = overlay->harvest(castle, std::min(capacity_remaining, rounded_amount));
	LOG_EMPERY_CENTER_DEBUG("Harvest: map_object_uuid = ", map_object_uuid,
		", map_object_type_id = ", map_object_type_id, ", harvest_speed = ", harvest_speed, ", interval = ", req.interval,
		", harvest_amount = ", harvest_amount, ", rounded_amount = ", rounded_amount, ", harvested_amount = ", harvested_amount);
	map_object->set_harvest_remainder(remainder); // noexcept

	return Response();
}

CLUSTER_SERVLET(Msg::KS_MapDeployImmigrants, cluster, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto map_object = WorldMap::get_map_object(map_object_uuid);
	if(!map_object){
		return Response(Msg::ERR_NO_SUCH_MAP_OBJECT) <<map_object_uuid;
	}
	const auto test_cluster = WorldMap::get_cluster(map_object->get_coord());
	if(cluster != test_cluster){
		return Response(Msg::ERR_MAP_OBJECT_ON_ANOTHER_CLUSTER);
	}

	const auto map_object_type_id = map_object->get_map_object_type_id();
	if(map_object_type_id != MapObjectTypeIds::ID_IMMIGRANTS){
		return Response(Msg::ERR_MAP_OBJECT_IS_NOT_IMMIGRANTS) <<map_object_type_id;
	}

	const auto coord = map_object->get_coord();
	auto result = can_deploy_castle_at(coord, map_object);
	if(result.first != 0){
		return std::move(result);
	}

	const auto castle_uuid = MapObjectUuid(Poseidon::Uuid::random());
	const auto owner_uuid = map_object->get_owner_uuid();
	const auto castle = boost::make_shared<Castle>(castle_uuid,
		owner_uuid, map_object->get_parent_object_uuid(), std::move(req.castle_name), coord);
	castle->pump_status();
	WorldMap::insert_map_object(castle);
	LOG_EMPERY_CENTER_INFO("Created castle: castle_uuid = ", castle_uuid, ", owner_uuid = ", owner_uuid);
	map_object->delete_from_game(); // noexcept

	return Response();
}

}
