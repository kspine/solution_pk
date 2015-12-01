#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/cs_map.hpp"
#include "../msg/sc_map.hpp"
#include "../msg/err_map.hpp"
#include <poseidon/json.hpp>
#include "../singletons/world_map.hpp"
#include "../utilities.hpp"
#include "../map_object.hpp"
#include "../cluster_session.hpp"
#include "../msg/ck_map.hpp"
#include "../data/map_object_type.hpp"
#include "../map_cell.hpp"
#include "../castle.hpp"
#include "../data/castle.hpp"
#include "../data/map_cell.hpp"
#include "../data/item.hpp"
#include "../singletons/item_box_map.hpp"
#include "../item_box.hpp"
#include "../transaction_element.hpp"
#include "../reason_ids.hpp"
#include "../map_object_type_ids.hpp"
#include "../data/global.hpp"

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
	const auto map_object_type_id = map_object->get_map_object_type_id();
	const auto map_object_type_data = Data::MapObjectType::require(map_object_type_id);
	const auto ms_per_cell = map_object_type_data->ms_per_cell;
	if(ms_per_cell == 0){
		return Response(Msg::ERR_NOT_MOVABLE_MAP_OBJECT) <<map_object_type_data->map_object_type_id;
	}

	const auto attack_target_uuid = MapObjectUuid(req.attack_target_uuid);

	auto from_coord = map_object->get_coord();
	const auto cluster = WorldMap::get_cluster(from_coord);
	if(!cluster){
		LOG_EMPERY_CENTER_WARNING("No cluster server available: from_coord = ", from_coord);
		return Response(Msg::ERR_CLUSTER_CONNECTION_LOST) <<from_coord;
	}

	Msg::CK_MapSetAction kreq;
	kreq.map_object_uuid = map_object->get_map_object_uuid().str();
	kreq.x = from_coord.x();
	kreq.y = from_coord.y();
	// 撤销当前的路径。
	auto kresult = cluster->send_and_wait(kreq);
	if(kresult.first != Msg::ST_OK){
		LOG_EMPERY_CENTER_DEBUG("Cluster server returned an error: code = ", kresult.first, ", msg = ", kresult.second);
		return std::move(kresult);
	}
	// 重新计算坐标。
	from_coord = map_object->get_coord();
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
	kresult = cluster->send_and_wait(kreq);
	if(kresult.first != Msg::ST_OK){
		LOG_EMPERY_CENTER_DEBUG("Cluster server returned an error: code = ", kresult.first, ", msg = ", kresult.second);
		return std::move(kresult);
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_MapPurchaseMapCell, account_uuid, session, req){
	const auto resource_id = ResourceId(req.resource_id);

	const auto &producible_resources = Data::Global::as_array(Data::Global::SLOT_PRODUCIBLE_RESOURCES);
	for(auto it = producible_resources.begin(); it != producible_resources.end(); ++it){
		if(resource_id == ResourceId(it->get<double>())){
			goto _producible;
		}
	}
	return Response(Msg::ERR_RESOURCE_NOT_PRODUCIBLE);
_producible:
	;

	const auto parent_object_uuid = MapObjectUuid(req.parent_object_uuid);
	const auto map_object = WorldMap::get_map_object(parent_object_uuid);
	if(!map_object){
		return Response(Msg::ERR_NO_SUCH_MAP_OBJECT) <<parent_object_uuid;
	}
	if(map_object->get_owner_uuid() != account_uuid){
		return Response(Msg::ERR_NOT_YOUR_MAP_OBJECT) <<map_object->get_owner_uuid();
	}
	const auto map_object_type_id = map_object->get_map_object_type_id();
	const auto castle = boost::dynamic_pointer_cast<Castle>(map_object);
	if(!castle){
		return Response(Msg::ERR_MAP_OBJECT_IS_NOT_A_CASTLE) <<map_object_type_id;
	}
	const auto coord = Coord(req.x, req.y);
	const auto cell_cluster_scope   = WorldMap::get_cluster_scope_by_coord(coord);
	const auto castle_cluster_scope = WorldMap::get_cluster_scope_by_coord(castle->get_coord());
	if(cell_cluster_scope.bottom_left() != castle_cluster_scope.bottom_left()){
		return Response(Msg::ERR_NOT_ON_THE_SAME_MAP_SERVER);
	}
	const auto castle_level = castle->get_level();
	const auto updrade_data = Data::CastleUpgradePrimary::require(castle_level);
	LOG_EMPERY_CENTER_DEBUG("Checking building upgrade data: castle_level = ", castle_level,
		", max_map_cell_count = ", updrade_data->max_map_cell_count, ", max_map_cell_distance = ", updrade_data->max_map_cell_distance);

	const auto current_cell_count = WorldMap::count_map_cells_by_parent_object(castle->get_map_object_uuid());
	if(current_cell_count >= updrade_data->max_map_cell_count){
		return Response(Msg::ERR_TOO_MANY_MAP_CELLS) <<current_cell_count;
	}
	const auto distance = get_distance_of_coords(coord, castle->get_coord());
	if(distance > updrade_data->max_map_cell_distance){
		return Response(Msg::ERR_MAP_CELL_IS_TOO_FAR_AWAY) <<distance;
	}

	const auto ticket_item_id = ItemId(req.ticket_item_id);
	const auto item_data = Data::Item::get(ticket_item_id);
	if(!item_data){
		return Response(Msg::ERR_LAND_PURCHASE_TICKET_NOT_FOUND) <<ticket_item_id;
	}
	if(item_data->type.first != Data::Item::CAT_LAND_PURCHASE_TICKET){
		return Response(Msg::ERR_BAD_LAND_PURCHASE_TICKET_TYPE) <<item_data->type.first;
	}

	const auto item_box = ItemBoxMap::require(account_uuid);

	auto map_cell = WorldMap::get_map_cell(coord);
	if(map_cell){
		if(map_cell->get_owner_uuid()){
			return Response(Msg::ERR_MAP_CELL_ALREADY_HAS_AN_OWNER) <<map_cell->get_owner_uuid();
		}
	} else {
		map_cell = boost::make_shared<MapCell>(coord);
		WorldMap::insert_map_cell(map_cell);
	}

	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(ItemTransactionElement::OP_REMOVE, ticket_item_id, 1,
		ReasonIds::ID_MAP_CELL_PURCHASE, coord.x(), coord.y(), 0);
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction.data(), transaction.size(),
		[&]{ map_cell->set_owner(parent_object_uuid, resource_id, ticket_item_id); });
	if(insuff_item_id){
		return Response(Msg::ERR_NO_LAND_PURCHASE_TICKET) <<insuff_item_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_MapUpgradeMapCell, account_uuid, session, req){
	const auto coord = Coord(req.x, req.y);
	const auto map_cell = WorldMap::get_map_cell(coord);
	if(!map_cell){
		return Response(Msg::ERR_NOT_YOUR_MAP_CELL) <<AccountUuid();
	}
	if(map_cell->get_owner_uuid() != account_uuid){
		return Response(Msg::ERR_NOT_YOUR_MAP_CELL) <<map_cell->get_owner_uuid();
	}

	const auto old_ticket_item_id = map_cell->get_ticket_item_id();
	if(!old_ticket_item_id){
		return Response(Msg::ERR_NO_TICKET_ON_MAP_CELL);
	}
	const auto old_ticket_data = Data::Item::require(old_ticket_item_id);
	const auto new_ticket_data = Data::Item::get_by_type(old_ticket_data->type.first, old_ticket_data->type.second + 1);
	if(!new_ticket_data){
		return Response(Msg::ERR_MAX_MAP_CELL_LEVEL_EXCEEDED);
	}
	const auto new_ticket_item_id = new_ticket_data->item_id;

	const auto item_box = ItemBoxMap::require(account_uuid);

	const auto trade_id = TradeId(Data::Global::as_unsigned(Data::Global::SLOT_MAP_CELL_UPGRADE_TRADE_ID));
	const auto trade_data = Data::ItemTrade::require(trade_id);
	std::vector<ItemTransactionElement> transaction;
	Data::unpack_item_trade(transaction, trade_data, 1, req.ID);
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction.data(), transaction.size(),
		[&]{ map_cell->set_ticket_item_id(new_ticket_item_id); });
	if(insuff_item_id){
		return Response(Msg::ERR_NO_LAND_UPGRADE_TICKET) <<insuff_item_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_MapDeployImmigrants, account_uuid, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto map_object = WorldMap::get_map_object(map_object_uuid);
	if(!map_object){
		return Response(Msg::ERR_NO_SUCH_MAP_OBJECT) <<map_object_uuid;
	}
	if(map_object->get_owner_uuid() != account_uuid){
		return Response(Msg::ERR_NOT_YOUR_MAP_OBJECT) <<map_object->get_owner_uuid();
	}
	const auto map_object_type_id = map_object->get_map_object_type_id();
	if(map_object_type_id != MapObjectTypeIds::ID_IMMIGRANTS){
		return Response(Msg::ERR_MAP_OBJECT_IS_NOT_IMMIGRANTS) <<map_object_type_id;
	}

	const auto castle_coord = map_object->get_coord();
	std::vector<Coord> foundation;
	get_castle_foundation(foundation, castle_coord);
	for(auto it = foundation.begin(); it != foundation.end(); ++it){
		const auto &coord = *it;
		const auto cluster_scope = WorldMap::get_cluster_scope_by_coord(coord);
		const auto map_x = static_cast<unsigned>(coord.x() - cluster_scope.left());
		const auto map_y = static_cast<unsigned>(coord.y() - cluster_scope.bottom());
		LOG_EMPERY_CENTER_DEBUG("Castle foundation: coord = ", coord, ", cluster_scope = ", cluster_scope,
			", map_x = ", map_x, ", map_y = ", map_y);
		const auto cell_data = Data::MapCellBasic::require(map_x, map_y);
		const auto terrain_data = Data::MapCellTerrain::require(cell_data->terrain_id);
		if(!terrain_data->buildable){
			return Response(Msg::ERR_CANNOT_DEPLOY_IMMIGRANTS_HERE) <<coord;
		}
	}
	// 检测与其他城堡距离。
	const auto min_distance = (boost::uint32_t)Data::Global::as_unsigned(Data::Global::SLOT_MINIMUM_DISTANCE_BETWEEN_CASTLES);

	const auto cluster_scope = WorldMap::get_cluster_scope_by_coord(castle_coord);
	const auto coll_left   = std::max(castle_coord.x() - (min_distance - 1), cluster_scope.left());
	const auto coll_bottom = std::max(castle_coord.y() - (min_distance - 1), cluster_scope.bottom());
	const auto coll_right  = std::min(castle_coord.x() + (min_distance + 2), cluster_scope.right());
	const auto coll_top    = std::max(castle_coord.y() + (min_distance + 2), cluster_scope.top());
	boost::container::flat_map<Coord, boost::shared_ptr<MapObject>> coll_map_objects;
	WorldMap::get_map_objects_by_rectangle(coll_map_objects, Rectangle(Coord(coll_left, coll_bottom), Coord(coll_right, coll_top)));
	for(auto it = coll_map_objects.begin(); it != coll_map_objects.end(); ++it){
		const auto &coord        = it->first;
		const auto &other_object = it->second;
		if(other_object->get_map_object_type_id() != MapObjectTypeIds::ID_CASTLE){
			continue;
		}
		LOG_EMPERY_CENTER_DEBUG("Checking distance: coord = ", coord, ", map_object_uuid = ", other_object->get_map_object_uuid());
		const auto distance = get_distance_of_coords(coord, castle_coord);
		if(distance <= min_distance){
			return Response(Msg::ERR_TOO_CLOSE_TO_ANOTHER_CASTLE) <<other_object->get_map_object_uuid();
		}
	}

	const auto castle_uuid = MapObjectUuid(Poseidon::Uuid::random());
	const auto castle = boost::make_shared<Castle>(castle_uuid,
		account_uuid, map_object->get_parent_object_uuid(), std::move(req.castle_name), castle_coord);
	castle->pump_status();
	WorldMap::insert_map_object(castle);
	LOG_EMPERY_CENTER_INFO("Created castle: castle_uuid = ", castle_uuid, ", account_uuid = ", account_uuid);
	map_object->delete_from_game(); // noexcept

	return Response();
}

}
