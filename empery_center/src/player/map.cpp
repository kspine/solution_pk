#include "../precompiled.hpp"
#include "common.hpp"
#include <poseidon/json.hpp>
#include "../msg/cs_map.hpp"
#include "../msg/sc_map.hpp"
#include "../msg/err_map.hpp"
#include "../singletons/world_map.hpp"
#include "../utilities.hpp"
#include "../map_object.hpp"
#include "../cluster_session.hpp"
#include "../msg/sk_map.hpp"
#include "../data/map_object_type.hpp"
#include "../map_cell.hpp"
#include "../castle.hpp"
#include "../data/castle.hpp"
#include "../data/map.hpp"
#include "../data/item.hpp"
#include "../singletons/item_box_map.hpp"
#include "../item_box.hpp"
#include "../transaction_element.hpp"
#include "../item_ids.hpp"
#include "../reason_ids.hpp"
#include "../data/global.hpp"
#include "../overlay.hpp"

namespace EmperyCenter {

PLAYER_SERVLET(Msg::CS_MapQueryWorldMap, account_uuid, session, /* req */){
	boost::container::flat_map<Coord, boost::shared_ptr<ClusterSession>> clusters;
	WorldMap::get_all_clusters(clusters);
	const auto center_rectangle = WorldMap::get_cluster_scope(Coord(0, 0));

	Msg::SC_MapWorldMapList msg;
	msg.clusters.reserve(clusters.size());
	for(auto it = clusters.begin(); it != clusters.end(); ++it){
		msg.clusters.emplace_back();
		auto &cluster = msg.clusters.back();
		cluster.x = it->first.x();
		cluster.y = it->first.y();
	}
	msg.cluster_width  = center_rectangle.width();
	msg.cluster_height = center_rectangle.height();
	session->send(msg);

	return Response();
}

PLAYER_SERVLET(Msg::CS_MapSetView, account_uuid, session, req){
	session->set_view(Rectangle(req.x, req.y, req.width, req.height));

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
	const auto speed = map_object_type_data->speed;
	if(speed <= 0){
		return Response(Msg::ERR_NOT_MOVABLE_MAP_OBJECT) <<map_object_type_id;
	}
	const auto ms_per_cell = static_cast<boost::uint64_t>(std::round(1000 / speed));

	auto from_coord = map_object->get_coord();
	const auto cluster = WorldMap::get_cluster(from_coord);
	if(!cluster){
		LOG_EMPERY_CENTER_DEBUG("No cluster server available: from_coord = ", from_coord);
		return Response(Msg::ERR_CLUSTER_CONNECTION_LOST) <<from_coord;
	}

	Msg::SK_MapSetAction kreq;
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
	kreq.action = req.action;
	kreq.param  = std::move(req.param);
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
	const auto cell_cluster_scope   = WorldMap::get_cluster_scope(coord);
	const auto castle_cluster_scope = WorldMap::get_cluster_scope(castle->get_coord());
	if(cell_cluster_scope.bottom_left() != castle_cluster_scope.bottom_left()){
		return Response(Msg::ERR_NOT_ON_THE_SAME_MAP_SERVER);
	}
	const auto castle_level = castle->get_level();
	const auto updrade_data = Data::CastleUpgradePrimary::require(castle_level);
	LOG_EMPERY_CENTER_DEBUG("Checking building upgrade data: castle_level = ", castle_level,
		", max_map_cell_count = ", updrade_data->max_map_cell_count, ", max_map_cell_distance = ", updrade_data->max_map_cell_distance);

	std::vector<boost::shared_ptr<MapCell>> current_cells;
	WorldMap::get_map_cells_by_parent_object(current_cells, parent_object_uuid);
	if(current_cells.size() >= updrade_data->max_map_cell_count){
		return Response(Msg::ERR_TOO_MANY_MAP_CELLS) <<current_cells.size();
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

	const auto map_cell = WorldMap::require_map_cell(coord);
	if(map_cell->get_owner_uuid()){
		return Response(Msg::ERR_MAP_CELL_ALREADY_HAS_AN_OWNER) <<map_cell->get_owner_uuid();
	}

	std::vector<boost::shared_ptr<Overlay>> overlays;
	WorldMap::get_overlays_by_rectangle(overlays, Rectangle(coord, 1, 1));
	for(auto it = overlays.begin(); it != overlays.end(); ++it){
		const auto &overlay = *it;
		if(!overlay->is_virtually_removed()){
			return Response(Msg::ERR_UNPURCHASABLE_WITH_OVERLAY) <<coord;
		}
	}

	const auto item_box = ItemBoxMap::require(account_uuid);

	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(ItemTransactionElement::OP_REMOVE, ticket_item_id, 1,
		ReasonIds::ID_MAP_CELL_PURCHASE, coord.x(), coord.y(), 0);
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction,
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
		return Response(Msg::ERR_NO_TICKET_ON_MAP_CELL) <<coord;
	}
	const auto old_ticket_data = Data::Item::require(old_ticket_item_id);
	const auto new_ticket_data = Data::Item::get_by_type(old_ticket_data->type.first, old_ticket_data->type.second + 1);
	if(!new_ticket_data){
		return Response(Msg::ERR_MAX_MAP_CELL_LEVEL_EXCEEDED);
	}
	const auto new_ticket_item_id = new_ticket_data->item_id;

	const auto item_box = ItemBoxMap::require(account_uuid);

	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(ItemTransactionElement::OP_REMOVE, ItemIds::ID_LAND_UPGRADE_TICKET, 1,
		ReasonIds::ID_MAP_CELL_UPGRADE, coord.x(), coord.y(), old_ticket_item_id.get());
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction,
		[&]{ map_cell->set_ticket_item_id(new_ticket_item_id); });
	if(insuff_item_id){
		return Response(Msg::ERR_NO_LAND_UPGRADE_TICKET) <<insuff_item_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_MapStopTroops, account_uuid, session, req){
	Msg::SC_MapStopTroopsRet msg;
	msg.map_objects.reserve(req.map_objects.size());
	for(auto it = req.map_objects.begin(); it != req.map_objects.end(); ++it){
		const auto map_object_uuid = MapObjectUuid(it->map_object_uuid);
		const auto map_object = WorldMap::get_map_object(map_object_uuid);
		if(!map_object){
			LOG_EMPERY_CENTER_DEBUG("No such map object: map_object_uuid = ", map_object_uuid);
			continue;
		}
		if(map_object->get_owner_uuid() != account_uuid){
			LOG_EMPERY_CENTER_DEBUG("Not your object: map_object_uuid = ", map_object_uuid);
			continue;
		}

		const auto from_coord = map_object->get_coord();
		const auto cluster = WorldMap::get_cluster(from_coord);
		if(!cluster){
			LOG_EMPERY_CENTER_WARNING("No cluster server available: from_coord = ", from_coord);
			continue;
		}

		Msg::SK_MapSetAction kreq;
		kreq.map_object_uuid = map_object->get_map_object_uuid().str();
		kreq.x = from_coord.x();
		kreq.y = from_coord.y();
		// 撤销当前的路径。
		const auto kresult = cluster->send_and_wait(kreq);
		if(kresult.first != Msg::ST_OK){
			LOG_EMPERY_CENTER_DEBUG("Cluster server returned an error: code = ", kresult.first, ", msg = ", kresult.second);
			continue;
		}

		msg.map_objects.emplace_back();
		auto &elem = msg.map_objects.back();
		elem.map_object_uuid = map_object_uuid.str();
	}
	session->send(msg);

	return Response();
}

PLAYER_SERVLET(Msg::CS_MapApplyAccelerationCard, account_uuid, session, req){
	const auto coord = Coord(req.x, req.y);
	const auto map_cell = WorldMap::get_map_cell(coord);
	if(!map_cell){
		return Response(Msg::ERR_NOT_YOUR_MAP_CELL) <<AccountUuid();
	}
	if(map_cell->get_owner_uuid() != account_uuid){
		return Response(Msg::ERR_NOT_YOUR_MAP_CELL) <<map_cell->get_owner_uuid();
	}

	const auto ticket_item_id = map_cell->get_ticket_item_id();
	if(!ticket_item_id){
		return Response(Msg::ERR_NO_TICKET_ON_MAP_CELL) <<coord;
	}
	if(map_cell->is_acceleration_card_applied()){
		return Response(Msg::ERR_ACCELERATION_CARD_APPLIED) <<coord;
	}

	const auto item_box = ItemBoxMap::require(account_uuid);

	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(ItemTransactionElement::OP_REMOVE, ItemIds::ID_ACCELERATION_CARD, 1,
		ReasonIds::ID_MAP_CELL_UPGRADE, coord.x(), coord.y(), ticket_item_id.get());
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction,
		[&]{ map_cell->set_acceleration_card_applied(true); });
	if(insuff_item_id){
		return Response(Msg::ERR_NO_LAND_UPGRADE_TICKET) <<insuff_item_id;
	}

	return Response();
}

}
