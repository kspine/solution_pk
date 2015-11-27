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
#include "../map_cell.hpp"
#include "../castle.hpp"
#include "../building_ids.hpp"
#include "../data/castle.hpp"
#include "../data/item.hpp"
#include "../singletons/item_box_map.hpp"
#include "../item_box.hpp"
#include "../transaction_element.hpp"
#include "../reason_ids.hpp"

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

	auto from_coord = map_object->get_coord();
	const auto cluster = WorldMap::get_cluster(from_coord);
	if(!cluster){
		LOG_EMPERY_CENTER_WARNING("No cluster server available: from_coord = ", from_coord);
		return Response(Msg::ERR_CLUSTER_CONNECTION_LOST) <<from_coord;
	}

	Msg::CK_MapSetWaypoints kreq;
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
	static constexpr ResourceId PRODUCIBLE_RESOURCES[] = { ResourceId(1101001), ResourceId(1101002), ResourceId(1101003) };

	const auto resource_id = ResourceId(req.resource_id);
	if(std::find(std::begin(PRODUCIBLE_RESOURCES), std::end(PRODUCIBLE_RESOURCES), resource_id) == std::end(PRODUCIBLE_RESOURCES)){
		return Response(Msg::ERR_RESOURCE_NOT_PRODUCIBLE);
	}

	const auto parent_object_uuid = MapObjectUuid(req.parent_object_uuid);
	const auto map_object = WorldMap::get_map_object(parent_object_uuid);
	if(!map_object){
		return Response(Msg::ERR_NO_SUCH_MAP_OBJECT) <<parent_object_uuid;
	}
	if(map_object->get_owner_uuid() != account_uuid){
		return Response(Msg::ERR_NOT_YOUR_MAP_OBJECT) <<map_object->get_owner_uuid();
	}
	const auto castle = boost::dynamic_pointer_cast<Castle>(map_object);
	if(!castle){
		return Response(Msg::ERR_MAP_OBJECT_IS_NOT_A_CASTLE) <<map_object->get_map_object_type_id();
	}
	std::vector<Castle::BuildingBaseInfo> primary_buildings;
	castle->get_buildings_by_id(primary_buildings, BuildingIds::ID_PRIMARY);
	if(primary_buildings.empty()){
		LOG_EMPERY_CENTER_ERROR("No primary buildign in castle? map_object_uuid = ", castle->get_map_object_uuid());
		DEBUG_THROW(Exception, sslit("No primary buildign in castle"));
	}
	const auto updrade_data = Data::CastleUpgradePrimary::require(primary_buildings.front().building_level);
	LOG_EMPERY_CENTER_DEBUG("Checking building upgrade data: building_level = ", updrade_data->building_level,
		", max_map_cell_count = ", updrade_data->max_map_cell_count, ", max_map_cell_distance = ", updrade_data->max_map_cell_distance);

	const auto current_cell_count = WorldMap::count_map_cells_by_parent_object(castle->get_map_object_uuid());
	if(current_cell_count >= updrade_data->max_map_cell_count){
		return Response(Msg::ERR_TOO_MANY_MAP_CELLS) <<current_cell_count;
	}
	const auto coord = Coord(req.x, req.y);
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
	static constexpr auto MAP_CELL_UPGRADE_TRADE_ID = TradeId(2804014);

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

	const auto trade_data = Data::ItemTrade::require(MAP_CELL_UPGRADE_TRADE_ID);
	std::vector<ItemTransactionElement> transaction;
	Data::unpack_item_trade(transaction, trade_data, 1, req.ID);
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction.data(), transaction.size(),
		[&]{ map_cell->set_ticket_item_id(new_ticket_item_id); });
	if(insuff_item_id){
		return Response(Msg::ERR_NO_LAND_UPGRADE_TICKET) <<insuff_item_id;
	}

	return Response();
}

}
