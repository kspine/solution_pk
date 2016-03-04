#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/cs_map.hpp"
#include "../msg/sc_map.hpp"
#include "../msg/err_map.hpp"
#include "../msg/err_castle.hpp"
#include "../msg/kill.hpp"
#include "../singletons/world_map.hpp"
#include "../map_utilities.hpp"
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
#include "../map_object_type_ids.hpp"
#include "../attribute_ids.hpp"
#include "../singletons/task_box_map.hpp"
#include "../task_box.hpp"
#include "../task_type_ids.hpp"

namespace EmperyCenter {

PLAYER_SERVLET(Msg::CS_MapQueryWorldMap, account, session, /* req */){
	boost::container::flat_map<Coord, boost::shared_ptr<ClusterSession>> clusters;
	WorldMap::get_all_clusters(clusters);
	const auto center_rectangle = WorldMap::get_cluster_scope(Coord(0, 0));

	Msg::SC_MapWorldMapList msg;
	msg.clusters.reserve(clusters.size());
	for(auto it = clusters.begin(); it != clusters.end(); ++it){
		auto &cluster = *msg.clusters.emplace(msg.clusters.end());
		cluster.x = it->first.x();
		cluster.y = it->first.y();
		cluster.name = it->second->get_name();
	}
	msg.cluster_width  = center_rectangle.width();
	msg.cluster_height = center_rectangle.height();
	session->send(msg);

	return Response();
}

PLAYER_SERVLET(Msg::CS_MapSetView, account, session, req){
	session->set_view(Rectangle(req.x, req.y, req.width, req.height));

	return Response();
}

PLAYER_SERVLET(Msg::CS_MapRefreshView, account, session, req){
	WorldMap::synchronize_player_view(session, Rectangle(req.x, req.y, req.width, req.height));

	return Response();
}

PLAYER_SERVLET(Msg::CS_MapSetWaypoints, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto map_object = WorldMap::get_map_object(map_object_uuid);
	if(!map_object){
		return Response(Msg::ERR_NO_SUCH_MAP_OBJECT) <<map_object_uuid;
	}
	if(map_object->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_YOUR_MAP_OBJECT) <<map_object->get_owner_uuid();
	}
	const auto map_object_type_id = map_object->get_map_object_type_id();
	const auto map_object_type_data = Data::MapObjectTypeAbstract::require(map_object_type_id);
	const auto speed = map_object_type_data->speed;
	if(speed <= 0){
		return Response(Msg::ERR_NOT_MOVABLE_MAP_OBJECT) <<map_object_type_id;
	}

	const auto ms_per_cell = static_cast<std::uint64_t>(std::round(1000 / speed));

	auto old_coord = map_object->get_coord();
	const auto cluster = WorldMap::get_cluster(old_coord);
	if(!cluster){
		LOG_EMPERY_CENTER_DEBUG("No cluster server available: old_coord = ", old_coord);
		return Response(Msg::ERR_CLUSTER_CONNECTION_LOST) <<old_coord;
	}

	Msg::SK_MapSetAction kreq;
	kreq.map_object_uuid = map_object->get_map_object_uuid().str();
	kreq.x = old_coord.x();
	kreq.y = old_coord.y();
	// 撤销当前的路径。
	auto kresult = cluster->send_and_wait(kreq);
	if(kresult.first != Msg::ST_OK){
		LOG_EMPERY_CENTER_WARNING("Cluster server returned an error: code = ", kresult.first, ", msg = ", kresult.second);
		// return std::move(kresult);
		cluster->shutdown(Msg::KILL_MAP_SERVER_RESYNCHRONIZE, "Lost map synchronization");
		return Response(Msg::ERR_CLUSTER_CONNECTION_LOST) <<old_coord;
	}
	// 重新计算坐标。
	old_coord = map_object->get_coord();
	kreq.x = old_coord.x();
	kreq.y = old_coord.y();
	kreq.waypoints.reserve(req.waypoints.size());
	auto last_coord = old_coord;
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

		auto &waypoint = *kreq.waypoints.emplace(kreq.waypoints.end());
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

PLAYER_SERVLET(Msg::CS_MapPurchaseMapCell, account, session, req){
	const auto item_box = ItemBoxMap::require(account->get_account_uuid());

	const auto resource_id = ResourceId(req.resource_id);

	const auto resource_data = Data::CastleResource::get(resource_id);
	if(!resource_data || !resource_data->producible){
		return Response(Msg::ERR_RESOURCE_NOT_PRODUCIBLE);
	}

	const auto parent_object_uuid = MapObjectUuid(req.parent_object_uuid);
	const auto map_object = WorldMap::get_map_object(parent_object_uuid);
	if(!map_object){
		return Response(Msg::ERR_NO_SUCH_MAP_OBJECT) <<parent_object_uuid;
	}
	if(map_object->get_owner_uuid() != account->get_account_uuid()){
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

	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(ItemTransactionElement::OP_REMOVE, ticket_item_id, 1,
		ReasonIds::ID_MAP_CELL_PURCHASE, coord.x(), coord.y(), 0);
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction, false,
		[&]{ map_cell->set_parent_object(parent_object_uuid, resource_id, ticket_item_id); });
	if(insuff_item_id){
		return Response(Msg::ERR_NO_LAND_PURCHASE_TICKET) <<insuff_item_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_MapUpgradeMapCell, account, session, req){
	const auto item_box = ItemBoxMap::require(account->get_account_uuid());

	const auto coord = Coord(req.x, req.y);
	const auto map_cell = WorldMap::get_map_cell(coord);
	if(!map_cell){
		return Response(Msg::ERR_NOT_YOUR_MAP_CELL) <<AccountUuid();
	}
	if(map_cell->get_owner_uuid() != account->get_account_uuid()){
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

	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(ItemTransactionElement::OP_REMOVE, ItemIds::ID_LAND_UPGRADE_TICKET, 1,
		ReasonIds::ID_MAP_CELL_UPGRADE, coord.x(), coord.y(), old_ticket_item_id.get());
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction, false,
		[&]{ map_cell->set_ticket_item_id(new_ticket_item_id); });
	if(insuff_item_id){
		return Response(Msg::ERR_NO_LAND_UPGRADE_TICKET) <<insuff_item_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_MapStopTroops, account, session, req){
	Msg::SC_MapStopTroopsRet msg;
	msg.map_objects.reserve(req.map_objects.size());
	for(auto it = req.map_objects.begin(); it != req.map_objects.end(); ++it){
		const auto map_object_uuid = MapObjectUuid(it->map_object_uuid);
		const auto map_object = WorldMap::get_map_object(map_object_uuid);
		if(!map_object){
			LOG_EMPERY_CENTER_DEBUG("No such map object: map_object_uuid = ", map_object_uuid);
			continue;
		}
		if(map_object->get_owner_uuid() != account->get_account_uuid()){
			LOG_EMPERY_CENTER_DEBUG("Not your object: map_object_uuid = ", map_object_uuid);
			continue;
		}

		const auto old_coord = map_object->get_coord();
		const auto cluster = WorldMap::get_cluster(old_coord);
		if(!cluster){
			LOG_EMPERY_CENTER_WARNING("No cluster server available: old_coord = ", old_coord);
			continue;
		}

		Msg::SK_MapSetAction kreq;
		kreq.map_object_uuid = map_object->get_map_object_uuid().str();
		kreq.x = old_coord.x();
		kreq.y = old_coord.y();
		// 撤销当前的路径。
		const auto kresult = cluster->send_and_wait(kreq);
		if(kresult.first != Msg::ST_OK){
			LOG_EMPERY_CENTER_WARNING("Cluster server returned an error: code = ", kresult.first, ", msg = ", kresult.second);
			cluster->shutdown(Msg::KILL_MAP_SERVER_RESYNCHRONIZE, "Lost map synchronization");
			continue;
		}

		auto &elem = *req.map_objects.emplace(req.map_objects.end());
		elem.map_object_uuid = map_object_uuid.str();
	}
	session->send(msg);

	return Response();
}

PLAYER_SERVLET(Msg::CS_MapApplyAccelerationCard, account, session, req){
	const auto item_box = ItemBoxMap::require(account->get_account_uuid());

	const auto coord = Coord(req.x, req.y);
	const auto map_cell = WorldMap::get_map_cell(coord);
	if(!map_cell){
		return Response(Msg::ERR_NOT_YOUR_MAP_CELL) <<AccountUuid();
	}
	if(map_cell->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_YOUR_MAP_CELL) <<map_cell->get_owner_uuid();
	}

	const auto ticket_item_id = map_cell->get_ticket_item_id();
	if(!ticket_item_id){
		return Response(Msg::ERR_NO_TICKET_ON_MAP_CELL) <<coord;
	}
	if(map_cell->is_acceleration_card_applied()){
		return Response(Msg::ERR_ACCELERATION_CARD_APPLIED) <<coord;
	}

	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(ItemTransactionElement::OP_REMOVE, ItemIds::ID_ACCELERATION_CARD, 1,
		ReasonIds::ID_MAP_CELL_UPGRADE, coord.x(), coord.y(), ticket_item_id.get());
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction, false,
		[&]{ map_cell->set_acceleration_card_applied(true); });
	if(insuff_item_id){
		return Response(Msg::ERR_NO_LAND_UPGRADE_TICKET) <<insuff_item_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_MapJumpToAnotherCluster, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto map_object = WorldMap::get_map_object(map_object_uuid);
	if(!map_object){
		return Response(Msg::ERR_NO_SUCH_MAP_OBJECT) <<map_object_uuid;
	}
	if(map_object->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_YOUR_MAP_OBJECT) <<map_object->get_owner_uuid();
	}
	const auto map_object_type_id = map_object->get_map_object_type_id();
	const auto map_object_type_data = Data::MapObjectTypeAbstract::require(map_object_type_id);
	const auto speed = map_object_type_data->speed;
	if(speed <= 0){
		return Response(Msg::ERR_NOT_MOVABLE_MAP_OBJECT) <<map_object_type_id;
	}

	auto old_coord = map_object->get_coord();
	const auto old_cluster = WorldMap::get_cluster(old_coord);
	if(!old_cluster){
		LOG_EMPERY_CENTER_DEBUG("No cluster server available: old_coord = ", old_coord);
		return Response(Msg::ERR_CLUSTER_CONNECTION_LOST) <<old_coord;
	}

	Msg::SK_MapSetAction kreq;
	kreq.map_object_uuid = map_object->get_map_object_uuid().str();
	kreq.x = old_coord.x();
	kreq.y = old_coord.y();
	// 撤销当前的路径。
	auto kresult = old_cluster->send_and_wait(kreq);
	if(kresult.first != Msg::ST_OK){
		LOG_EMPERY_CENTER_WARNING("Cluster server returned an error: code = ", kresult.first, ", msg = ", kresult.second);
		old_cluster->shutdown(Msg::KILL_MAP_SERVER_RESYNCHRONIZE, "Lost map synchronization");
		return Response(Msg::ERR_CLUSTER_CONNECTION_LOST) <<old_coord;
	}
	// 重新计算坐标。
	old_coord = map_object->get_coord();
	const auto old_cluster_scope = WorldMap::get_cluster_scope(old_coord);

	const unsigned border_thickness  = Data::Global::as_unsigned(Data::Global::SLOT_MAP_BORDER_THICKNESS);
	const unsigned bypassable_blocks = Data::Global::as_unsigned(Data::Global::SLOT_MAP_SWITCH_MAX_BYPASSABLE_BLOCKS);

	const auto old_cluster_coord = old_cluster_scope.bottom_left();
	const auto old_map_x = static_cast<unsigned>(old_coord.x() - old_cluster_coord.x());
	const auto old_map_y = static_cast<unsigned>(old_coord.y() - old_cluster_coord.y());

	auto new_cluster_coord = old_cluster_coord;
	auto new_map_x = old_map_x;
	auto new_map_y = old_map_y;

	int bypass_dx = 0;
	int bypass_dy = 0;
	{
		const auto direction = req.direction % 4;
		LOG_EMPERY_CENTER_DEBUG("Checking map border: old_coord = ", old_coord, ", direction = ", direction);
		switch(direction){
		case 0:  // x+
			if(old_map_x != old_cluster_scope.width() - border_thickness - 1){
				LOG_EMPERY_CENTER_DEBUG("> Not at right edge: old_map_x = ", old_map_x);
				return Response(Msg::ERR_NOT_AT_MAP_EDGE);
			}
			new_cluster_coord = Coord(old_cluster_coord.x() + static_cast<std::int64_t>(old_cluster_scope.width()),
			                          old_cluster_coord.y());
			new_map_x = border_thickness;
			bypass_dx = 1;
			break;

		case 1:  // y+
			if(old_map_y != old_cluster_scope.height() - border_thickness - 1){
				LOG_EMPERY_CENTER_DEBUG("> Not at top edge: old_map_y = ", old_map_y);
				return Response(Msg::ERR_NOT_AT_MAP_EDGE);
			}
			new_cluster_coord = Coord(old_cluster_coord.x(),
			                          old_cluster_coord.y() + static_cast<std::int64_t>(old_cluster_scope.height()));
			new_map_y = border_thickness;
			bypass_dy = 1;
			break;

		case 2:  // x-
			if(old_map_x != border_thickness){
				LOG_EMPERY_CENTER_DEBUG("> Not at left edge: old_map_x = ", old_map_x);
				return Response(Msg::ERR_NOT_AT_MAP_EDGE);
			}
			new_cluster_coord = Coord(old_cluster_coord.x() - static_cast<std::int64_t>(old_cluster_scope.width()),
			                          old_cluster_coord.y());
			new_map_x = old_cluster_scope.width() - border_thickness - 1;
			bypass_dx = -1;
			break;

		default: // y-
			if(old_map_y != border_thickness){
				LOG_EMPERY_CENTER_DEBUG("> Not at bottom edge: old_map_y = ", old_map_y);
				return Response(Msg::ERR_NOT_AT_MAP_EDGE);
			}
			new_cluster_coord = Coord(old_cluster_coord.x(),
			                          old_cluster_coord.y() - static_cast<std::int64_t>(old_cluster_scope.height()));
			new_map_y = old_cluster_scope.height() - border_thickness - 1;
			bypass_dy = -1;
			break;
		}
	}

	const auto new_cluster = WorldMap::get_cluster(new_cluster_coord);
	if(!new_cluster){
		LOG_EMPERY_CENTER_DEBUG("No cluster server available: new_cluster_coord = ", new_cluster_coord);
		return Response(Msg::ERR_CLUSTER_CONNECTION_LOST) <<new_cluster_coord;
	}

	const auto try_once = [&](Coord new_coord) -> std::pair<long, std::string> {
		const auto new_map_cell = WorldMap::get_map_cell(new_coord);
		if(new_map_cell){
			const auto cell_owner_uuid = new_map_cell->get_owner_uuid();
			if(cell_owner_uuid && (account->get_account_uuid() != cell_owner_uuid)){
				LOG_EMPERY_CENTER_DEBUG("Blocked by a cell owned by another player's territory: cell_owner_uuid = ", cell_owner_uuid);
				return Response(Msg::ERR_BLOCKED_BY_OTHER_TERRITORY) <<cell_owner_uuid;
			}
		}

		const auto cell_data = Data::MapCellBasic::require(new_map_x, new_map_y);
		const auto terrain_id = cell_data->terrain_id;
		const auto terrain_data = Data::MapTerrain::require(terrain_id);
		if(!terrain_data->passable){
			LOG_EMPERY_CENTER_DEBUG("Blocked by terrain: terrain_id = ", terrain_id);
			return Response(Msg::ERR_BLOCKED_BY_IMPASSABLE_MAP_CELL) <<terrain_id;
		}

		std::vector<boost::shared_ptr<MapObject>> adjacent_objects;
		WorldMap::get_map_objects_by_rectangle(adjacent_objects,
			Rectangle(Coord(new_coord.x() - 3, new_coord.y() - 3), Coord(new_coord.x() + 4, new_coord.y() + 4)));
		std::vector<Coord> foundation;
		for(auto it = adjacent_objects.begin(); it != adjacent_objects.end(); ++it){
			const auto &other_object = *it;
			const auto other_map_object_uuid = other_object->get_map_object_uuid();
			const auto other_coord = other_object->get_coord();
			if(new_coord == other_coord){
				LOG_EMPERY_CENTER_DEBUG("Blocked by another map object: other_map_object_uuid = ", other_map_object_uuid);
				return Response(Msg::ERR_BLOCKED_BY_TROOPS) <<other_map_object_uuid;
			}
			const auto other_object_type_id = other_object->get_map_object_type_id();
			if(other_object_type_id != EmperyCenter::MapObjectTypeIds::ID_CASTLE){
				continue;
			}
			foundation.clear();
			get_castle_foundation(foundation, other_coord, true);
			for(auto fit = foundation.begin(); fit != foundation.end(); ++fit){
				if(new_coord == *fit){
					LOG_EMPERY_CENTER_DEBUG("Blocked by castle: other_map_object_uuid = ", other_map_object_uuid);
					return Response(Msg::ERR_BLOCKED_BY_CASTLE) <<other_map_object_uuid;
				}
			}
		}
		return Response();
	};

	unsigned bypass_count = 0;
	auto new_coord = Coord(new_cluster_coord.x() + new_map_x, new_cluster_coord.y() + new_map_y);
	for(;;){
		auto result = try_once(new_coord);
		if(result.first == Msg::ST_OK){
			break;
		}

		if(bypass_count >= bypassable_blocks){
			LOG_EMPERY_CENTER_DEBUG("Max bypass block count exceeded. Give up.");
			return std::move(result);
		}
		++bypass_count;

		new_coord = Coord(new_coord.x() + bypass_dx, new_coord.y() + bypass_dy);
		LOG_EMPERY_CENTER_DEBUG("Trying next point: new_coord = ", new_coord);
	}

	LOG_EMPERY_CENTER_DEBUG("Jump to another cluster: map_object_uuid = ", map_object_uuid,
		", old_cood = ", old_coord, ", new_coord = ", new_coord);
	map_object->set_action(0, { });
	map_object->set_coord(new_coord);

	session->send(Msg::SC_MapObjectStopped(map_object_uuid.str(), 0, std::string(), Msg::ERR_SWITCHED_CLUSTER, std::string()));

	return Response();
}

PLAYER_SERVLET(Msg::CS_MapDismissBattalion, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto map_object = WorldMap::get_map_object(map_object_uuid);
	if(!map_object){
		return Response(Msg::ERR_NO_SUCH_MAP_OBJECT) <<map_object_uuid;
	}
	if(map_object->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_YOUR_MAP_OBJECT) <<map_object->get_owner_uuid();
	}
	const auto map_object_type_id = map_object->get_map_object_type_id();
	const auto map_object_type_data = Data::MapObjectTypeBattalion::require(map_object_type_id);
	const auto speed = map_object_type_data->speed;
	if(speed <= 0){
		return Response(Msg::ERR_NOT_MOVABLE_MAP_OBJECT) <<map_object_type_id;
	}

	const auto castle_uuid = map_object->get_parent_object_uuid();
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(castle_uuid));
	if(castle && map_object->is_garrisoned()){
		auto soldier_count = map_object->get_attribute(AttributeIds::ID_SOLDIER_COUNT);
		if(soldier_count < 0){
			soldier_count = 0;
		}

		const auto castle_uuid_head    = Poseidon::load_be(reinterpret_cast<const std::uint64_t &>(castle_uuid.get()[0]));
		const auto battalion_uuid_head = Poseidon::load_be(reinterpret_cast<const std::uint64_t &>(map_object_uuid.get()[0]));

		std::vector<SoldierTransactionElement> transaction;
		transaction.emplace_back(SoldierTransactionElement::OP_ADD, map_object_type_id, soldier_count,
			ReasonIds::ID_DISMISS_BATTALION, castle_uuid_head, battalion_uuid_head, 0);
		castle->commit_soldier_transaction(transaction,
			[&]{ map_object->delete_from_game(); });
	} else {
		map_object->delete_from_game();
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_MapEvacuateFromCastle, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto map_object = WorldMap::get_map_object(map_object_uuid);
	if(!map_object){
		return Response(Msg::ERR_NO_SUCH_MAP_OBJECT) <<map_object_uuid;
	}
	if(map_object->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_YOUR_MAP_OBJECT) <<map_object->get_owner_uuid();
	}
	const auto map_object_type_id = map_object->get_map_object_type_id();
	const auto map_object_type_data = Data::MapObjectTypeBattalion::require(map_object_type_id);
	const auto speed = map_object_type_data->speed;
	if(speed <= 0){
		return Response(Msg::ERR_NOT_MOVABLE_MAP_OBJECT) <<map_object_type_id;
	}

	const auto castle_uuid = map_object->get_parent_object_uuid();
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(castle_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<castle_uuid;
	}
	if(!map_object->is_garrisoned()){
		return Response(Msg::ERR_MAP_OBJECT_IS_NOT_GARRISONED);
	}

	std::vector<Coord> foundation;
	get_castle_foundation(foundation, castle->get_coord(), false);
	for(;;){
		if(foundation.empty()){
			return Response(Msg::ERR_NO_ROOM_FOR_NEW_UNIT);
		}
		const auto &coord = foundation.front();
		std::vector<boost::shared_ptr<MapObject>> test_objects;
		WorldMap::get_map_objects_by_rectangle(test_objects, Rectangle(coord, 1, 1));
		if(test_objects.empty()){
			LOG_EMPERY_CENTER_DEBUG("Found coord for battalion: coord = ", coord);
			break;
		}
		foundation.erase(foundation.begin());
	}
	const auto &coord = foundation.front();

	map_object->set_coord(coord);
	map_object->set_garrisoned(false);

	return Response();
}

PLAYER_SERVLET(Msg::CS_MapRefillBattalion, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto map_object = WorldMap::get_map_object(map_object_uuid);
	if(!map_object){
		return Response(Msg::ERR_NO_SUCH_MAP_OBJECT) <<map_object_uuid;
	}
	if(map_object->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_YOUR_MAP_OBJECT) <<map_object->get_owner_uuid();
	}
	const auto map_object_type_id = map_object->get_map_object_type_id();
	const auto map_object_type_data = Data::MapObjectTypeBattalion::require(map_object_type_id);
	const auto speed = map_object_type_data->speed;
	if(speed <= 0){
		return Response(Msg::ERR_NOT_MOVABLE_MAP_OBJECT) <<map_object_type_id;
	}

	const auto castle_uuid = map_object->get_parent_object_uuid();
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(castle_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<castle_uuid;
	}
	if(!map_object->is_garrisoned()){
		return Response(Msg::ERR_MAP_OBJECT_IS_NOT_GARRISONED);
	}

	const auto soldier_count = req.soldier_count;
	if(soldier_count == 0){
		return Response(Msg::ERR_ZERO_SOLDIER_COUNT);
	}

	const auto old_soldier_count = static_cast<std::uint64_t>(map_object->get_attribute(AttributeIds::ID_SOLDIER_COUNT));
	const auto new_soldier_count = checked_add(old_soldier_count, soldier_count);
	const auto max_soldier_count = map_object_type_data->max_soldier_count;
	if(new_soldier_count > max_soldier_count){
		return Response(Msg::ERR_TOO_MANY_SOLDIERS_FOR_BATTALION) <<max_soldier_count;
	}

	const auto castle_uuid_head    = Poseidon::load_be(reinterpret_cast<const std::uint64_t &>(castle_uuid.get()[0]));
	const auto battalion_uuid_head = Poseidon::load_be(reinterpret_cast<const std::uint64_t &>(map_object_uuid.get()[0]));

	boost::container::flat_map<AttributeId, std::int64_t> modifiers;
	modifiers[AttributeIds::ID_SOLDIER_COUNT] = static_cast<std::int64_t>(new_soldier_count);
	const auto old_soldier_count_max = static_cast<std::uint64_t>(map_object->get_attribute(AttributeIds::ID_SOLDIER_COUNT_MAX));
	if(new_soldier_count > old_soldier_count_max){
		modifiers[AttributeIds::ID_SOLDIER_COUNT_MAX] = static_cast<std::int64_t>(new_soldier_count);
	}

	std::vector<SoldierTransactionElement> transaction;
	transaction.emplace_back(SoldierTransactionElement::OP_REMOVE, map_object_type_id, soldier_count,
		ReasonIds::ID_REFILL_BATTALION, castle_uuid_head, battalion_uuid_head, 0);
	const auto insuff_battalion_id = castle->commit_soldier_transaction_nothrow(transaction,
		[&]{ map_object->set_attributes(std::move(modifiers)); });
	if(insuff_battalion_id){
		return Response(Msg::ERR_CASTLE_NO_ENOUGH_SOLDIERS) <<insuff_battalion_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_MapLoadMinimap, account, session, req){
	const auto rectangle = Rectangle(req.left, req.bottom, req.width, req.height);

	std::vector<boost::shared_ptr<MapObject>> map_objects;
	WorldMap::get_map_objects_by_rectangle(map_objects, rectangle);

	Msg::SC_MapMinimap msg;
	msg.left   = rectangle.left();
	msg.bottom = rectangle.bottom();
	msg.width  = rectangle.width();
	msg.height = rectangle.height();
	msg.castles.reserve(256);
	for(auto it = map_objects.begin(); it != map_objects.end(); ++it){
		const auto &map_object = *it;
		if(map_object->get_map_object_type_id() != MapObjectTypeIds::ID_CASTLE){
			continue;
		}
		auto &elem = *msg.castles.emplace(msg.castles.end());
		elem.x          = map_object->get_coord().x();
		elem.y          = map_object->get_coord().y();
		elem.owner_uuid = map_object->get_owner_uuid().str();
	}
	session->send(msg);

	return Response();
}

PLAYER_SERVLET(Msg::CS_MapHarvestMapCell, account, session, req){
	const auto task_box = TaskBoxMap::require(account->get_account_uuid());

	const auto coord = Coord(req.x, req.y);
	const auto map_cell = WorldMap::get_map_cell(coord);
	if(!map_cell){
		return Response(Msg::ERR_NOT_YOUR_MAP_CELL) <<AccountUuid();
	}
	if(map_cell->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_YOUR_MAP_CELL) <<map_cell->get_owner_uuid();
	}

	const auto parent_object_uuid = map_cell->get_parent_object_uuid();
	const auto map_object = WorldMap::get_map_object(parent_object_uuid);
	if(!map_object){
		return Response(Msg::ERR_NO_SUCH_MAP_OBJECT) <<parent_object_uuid;
	}
	if(map_object->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_YOUR_MAP_OBJECT) <<map_object->get_owner_uuid();
	}
	const auto map_object_type_id = map_object->get_map_object_type_id();
	const auto castle = boost::dynamic_pointer_cast<Castle>(map_object);
	if(!castle){
		return Response(Msg::ERR_MAP_OBJECT_IS_NOT_A_CASTLE) <<map_object_type_id;
	}

	map_cell->pump_status();

	if(map_cell->get_resource_amount() != 0){
		const auto resource_id = map_cell->get_production_resource_id();
		const auto amount_harvested = map_cell->harvest(false);
		if(amount_harvested == 0){
			return Response(Msg::ERR_WAREHOUSE_FULL);
		}
		try {
			task_box->check(TaskTypeIds::ID_HARVEST_RESOURCE, resource_id.get(), amount_harvested,
				castle, 0, 0, 0);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
		}
	}

	return Response();
}

}
