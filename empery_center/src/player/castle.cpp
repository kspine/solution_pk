#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/cs_castle.hpp"
#include "../msg/err_castle.hpp"
#include "../singletons/world_map.hpp"
#include "../castle.hpp"
#include "../data/castle.hpp"
#include "../transaction_element.hpp"
#include "../reason_ids.hpp"
#include "../map_object_type_ids.hpp"
#include "../map_cell.hpp"
#include "../data/global.hpp"
#include "../singletons/item_box_map.hpp"
#include "../item_box.hpp"
#include "../data/item.hpp"
#include "../msg/err_item.hpp"
#include "../utilities.hpp"

namespace EmperyCenter {

PLAYER_SERVLET(Msg::CS_CastleQueryInfo, account_uuid, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account_uuid){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	castle->pump_status();
	castle->synchronize_with_player(session);

	std::vector<boost::shared_ptr<MapObject>> child_objects;
	WorldMap::get_map_objects_by_parent_object(child_objects, map_object_uuid);
	for(auto it = child_objects.begin(); it != child_objects.end(); ++it){
		const auto &child = *it;
		LOG_EMPERY_CENTER_DEBUG("Child object: map_object_uuid = ", map_object_uuid,
			", child_object_uuid = ", child->get_map_object_uuid(), ", child_type_id = ", child->get_map_object_type_id());
		child->pump_status();
		child->synchronize_with_player(session);
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleCreateBuilding, account_uuid, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account_uuid){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	castle->pump_status();

	const auto queue_size = castle->get_building_queue_size();
	const auto max_queue_size = Data::Global::as_unsigned(Data::Global::SLOT_MAX_CONCURRENT_UPGRADING_BUILDING_COUNT);
	if(queue_size >= max_queue_size){
		return Response(Msg::ERR_BUILDING_QUEUE_FULL) <<max_queue_size;
	}

	const auto building_base_id = BuildingBaseId(req.building_base_id);
	// castle->pump_building_status(building_base_id);

	const auto info = castle->get_building_base(building_base_id);
	if(info.mission != Castle::MIS_NONE){
		return Response(Msg::ERR_ANOTHER_BUILDING_THERE) <<building_base_id;
	}

	const auto building_id = BuildingId(req.building_id);
	const auto building_data = Data::CastleBuilding::get(building_id);
	if(!building_data){
		return Response(Msg::ERR_NO_SUCH_BUILDING) <<building_id;
	}
	const auto count = castle->count_buildings_by_id(building_id);
	if(count >= building_data->build_limit){
		return Response(Msg::ERR_BUILD_LIMIT_EXCEEDED) <<building_id;
	}

	const auto building_base_data = Data::CastleBuildingBase::get(building_base_id);
	if(!building_base_data){
		return Response(Msg::ERR_NO_SUCH_CASTLE_BASE) <<building_base_id;
	}
	if(building_base_data->buildings_allowed.find(building_data->building_id) == building_base_data->buildings_allowed.end()){
		return Response(Msg::ERR_BUILDING_NOT_ALLOWED) <<building_id;
	}

	const auto upgrade_data = Data::CastleUpgradeAbstract::require(building_data->type, 1);
	for(auto it = upgrade_data->prerequisite.begin(); it != upgrade_data->prerequisite.end(); ++it){
		std::vector<Castle::BuildingBaseInfo> prerequisite_buildings;
		castle->get_buildings_by_id(prerequisite_buildings, it->first);
		unsigned max_level = 0;
		for(auto prereq_it = prerequisite_buildings.begin(); prereq_it != prerequisite_buildings.end(); ++prereq_it){
			if(max_level < prereq_it->building_level){
				max_level = prereq_it->building_level;
			}
		}
		if(max_level < it->second){
			LOG_EMPERY_CENTER_DEBUG("Prerequisite not met: building_id = ", it->first,
				", level_required = ", it->second, ", max_level = ", max_level);
			return Response(Msg::ERR_PREREQUISITE_NOT_MET) <<it->first;
		}
	}
	std::vector<ResourceTransactionElement> transaction;
	for(auto it = upgrade_data->upgrade_cost.begin(); it != upgrade_data->upgrade_cost.end(); ++it){
		transaction.emplace_back(ResourceTransactionElement::OP_REMOVE, it->first, it->second,
			ReasonIds::ID_UPGRADE_BUILDING, building_data->building_id.get(), upgrade_data->building_level, 0);
	}
	const auto insuff_resource_id = castle->commit_resource_transaction_nothrow(transaction,
		[&]{ castle->create_building_mission(building_base_id, Castle::MIS_CONSTRUCT, building_data->building_id); });
	if(insuff_resource_id){
		return Response(Msg::ERR_CASTLE_NO_ENOUGH_RESOURCES) <<insuff_resource_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleCancelBuildingMission, account_uuid, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account_uuid){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	const auto building_base_id = BuildingBaseId(req.building_base_id);
	castle->pump_building_status(building_base_id);

	const auto info = castle->get_building_base(building_base_id);
	if(info.mission == Castle::MIS_NONE){
		return Response(Msg::ERR_NO_BUILDING_MISSION) <<building_base_id;
	}

	const auto building_data = Data::CastleBuilding::require(info.building_id);
	const auto upgrade_data = Data::CastleUpgradeAbstract::require(building_data->type, info.building_level + 1);
	std::vector<ResourceTransactionElement> transaction;
	if((info.mission == Castle::MIS_CONSTRUCT) || (info.mission == Castle::MIS_UPGRADE)){
		const auto refund_ratio = Data::Global::as_double(Data::Global::SLOT_CASTLE_CANCELLATION_REFUND_RATIO);
		for(auto it = upgrade_data->upgrade_cost.begin(); it != upgrade_data->upgrade_cost.end(); ++it){
			transaction.emplace_back(ResourceTransactionElement::OP_ADD,
				it->first, static_cast<boost::uint64_t>(std::floor(it->second * refund_ratio + 0.001)),
				ReasonIds::ID_CANCEL_UPGRADE_BUILDING, info.building_id.get(), info.building_level, 0);
		}
	}
	castle->commit_resource_transaction(transaction,
		[&]{ castle->cancel_building_mission(building_base_id); });

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleUpgradeBuilding, account_uuid, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account_uuid){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	castle->pump_status();

	const auto queue_size = castle->get_building_queue_size();
	const auto max_queue_size = Data::Global::as_unsigned(Data::Global::SLOT_MAX_CONCURRENT_UPGRADING_BUILDING_COUNT);
	if(queue_size >= max_queue_size){
		return Response(Msg::ERR_BUILDING_QUEUE_FULL) <<max_queue_size;
	}

	const auto building_base_id = BuildingBaseId(req.building_base_id);
	// castle->pump_building_status(building_base_id);

	const auto info = castle->get_building_base(building_base_id);
	if(info.building_id == BuildingId()){
		return Response(Msg::ERR_NO_BUILDING_THERE) <<building_base_id;
	}
	if(info.mission != Castle::MIS_NONE){
		return Response(Msg::ERR_BUILDING_MISSION_CONFLICT) <<building_base_id;
	}

	const auto building_data = Data::CastleBuilding::require(info.building_id);
	const auto upgrade_data = Data::CastleUpgradeAbstract::get(building_data->type, info.building_level + 1);
	if(!upgrade_data){
		return Response(Msg::ERR_BUILDING_UPGRADE_MAX) <<info.building_id;
	}
	for(auto it = upgrade_data->prerequisite.begin(); it != upgrade_data->prerequisite.end(); ++it){
		std::vector<Castle::BuildingBaseInfo> prerequisite_buildings;
		castle->get_buildings_by_id(prerequisite_buildings, it->first);
		unsigned max_level = 0;
		for(auto prereq_it = prerequisite_buildings.begin(); prereq_it != prerequisite_buildings.end(); ++prereq_it){
			if(max_level < prereq_it->building_level){
				max_level = prereq_it->building_level;
			}
		}
		if(max_level < it->second){
			LOG_EMPERY_CENTER_DEBUG("Prerequisite not met: building_id = ", it->first,
				", level_required = ", it->second, ", max_level = ", max_level);
			return Response(Msg::ERR_PREREQUISITE_NOT_MET) <<it->first;
		}
	}
	std::vector<ResourceTransactionElement> transaction;
	for(auto it = upgrade_data->upgrade_cost.begin(); it != upgrade_data->upgrade_cost.end(); ++it){
		transaction.emplace_back(ResourceTransactionElement::OP_REMOVE, it->first, it->second,
			ReasonIds::ID_UPGRADE_BUILDING, building_data->building_id.get(), upgrade_data->building_level, 0);
	}
	const auto insuff_resource_id = castle->commit_resource_transaction_nothrow(transaction,
		[&]{ castle->create_building_mission(building_base_id, Castle::MIS_UPGRADE, { }); });
	if(insuff_resource_id){
		return Response(Msg::ERR_CASTLE_NO_ENOUGH_RESOURCES) <<insuff_resource_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleDestroyBuilding, account_uuid, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account_uuid){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	castle->pump_status();

	const auto queue_size = castle->get_building_queue_size();
	const auto max_queue_size = Data::Global::as_unsigned(Data::Global::SLOT_MAX_CONCURRENT_UPGRADING_BUILDING_COUNT);
	if(queue_size >= max_queue_size){
		return Response(Msg::ERR_BUILDING_QUEUE_FULL) <<max_queue_size;
	}

	const auto building_base_id = BuildingBaseId(req.building_base_id);
	// castle->pump_building_status(building_base_id);

	const auto info = castle->get_building_base(building_base_id);
	if(info.building_id == BuildingId()){
		return Response(Msg::ERR_NO_BUILDING_THERE) <<building_base_id;
	}
	if(info.mission != Castle::MIS_NONE){
		return Response(Msg::ERR_BUILDING_MISSION_CONFLICT) <<building_base_id;
	}

	const auto building_data = Data::CastleBuilding::require(info.building_id);
	const auto upgrade_data = Data::CastleUpgradeAbstract::require(building_data->type, info.building_level);
	if(upgrade_data->destruct_duration == 0){
		return Response(Msg::ERR_BUILDING_NOT_DESTRUCTIBLE) <<info.building_id;
	}

	castle->create_building_mission(building_base_id, Castle::MIS_DESTRUCT, BuildingId());

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleCompleteBuildingImmediately, account_uuid, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account_uuid){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	const auto building_base_id = BuildingBaseId(req.building_base_id);
	castle->pump_building_status(building_base_id);

	const auto info = castle->get_building_base(building_base_id);
	if(info.mission == Castle::MIS_NONE){
		return Response(Msg::ERR_NO_BUILDING_MISSION) <<building_base_id;
	}

	const auto utc_now = Poseidon::get_utc_time();

	const auto item_box = ItemBoxMap::require(account_uuid);

	const auto trade_id = TradeId(Data::Global::as_unsigned(Data::Global::SLOT_CASTLE_BUILDING_IMMEDIATE_UPGRADE_TRADE_ID));
	const auto trade_data = Data::ItemTrade::require(trade_id);
	const auto time_remaining = saturated_sub(info.mission_time_end, utc_now);
	const auto trade_count = static_cast<boost::uint64_t>(std::ceil(time_remaining / 60000.0));
	std::vector<ItemTransactionElement> transaction;
	Data::unpack_item_trade(transaction, trade_data, trade_count, req.ID);
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction,
		[&]{ castle->speed_up_building_mission(building_base_id, UINT64_MAX); });
	if(insuff_item_id){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<insuff_item_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleQueryIndividualBuildingInfo, account_uuid, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account_uuid){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	const auto building_base_id = BuildingBaseId(req.building_base_id);
	castle->pump_building_status(building_base_id);
	castle->synchronize_building_with_player(building_base_id, session);

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleUpgradeTech, account_uuid, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account_uuid){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	castle->pump_status();

	const auto queue_size = castle->get_tech_queue_size();
	const auto max_queue_size = Data::Global::as_unsigned(Data::Global::SLOT_MAX_CONCURRENT_UPGRADING_TECH_COUNT);
	if(queue_size >= max_queue_size){
		return Response(Msg::ERR_TECH_QUEUE_FULL) <<max_queue_size;
	}

	const auto tech_id = TechId(req.tech_id);
	// castle->pump_tech_status(tech_id);

	const auto info = castle->get_tech(tech_id);
	if(info.mission != Castle::MIS_NONE){
		return Response(Msg::ERR_TECH_MISSION_CONFLICT) <<tech_id;
	}

	const auto tech_data = Data::CastleTech::get(TechId(req.tech_id), info.tech_level + 1);
	if(!tech_data){
		if(info.tech_level == 0){
			return Response(Msg::ERR_NO_SUCH_TECH) <<tech_id;
		}
		return Response(Msg::ERR_TECH_UPGRADE_MAX) <<tech_id;
	}

	for(auto it = tech_data->prerequisite.begin(); it != tech_data->prerequisite.end(); ++it){
		std::vector<Castle::BuildingBaseInfo> prerequisite_buildings;
		castle->get_buildings_by_id(prerequisite_buildings, it->first);
		unsigned max_level = 0;
		for(auto prereq_it = prerequisite_buildings.begin(); prereq_it != prerequisite_buildings.end(); ++prereq_it){
			if(max_level < prereq_it->building_level){
				max_level = prereq_it->building_level;
			}
		}
		if(max_level < it->second){
			LOG_EMPERY_CENTER_DEBUG("Prerequisite not met: tech_id = ", it->first,
				", level_required = ", it->second, ", max_level = ", max_level);
			return Response(Msg::ERR_PREREQUISITE_NOT_MET) <<it->first;
		}
	}
	for(auto it = tech_data->display_prerequisite.begin(); it != tech_data->display_prerequisite.end(); ++it){
		std::vector<Castle::BuildingBaseInfo> prerequisite_buildings;
		castle->get_buildings_by_id(prerequisite_buildings, it->first);
		unsigned max_level = 0;
		for(auto prereq_it = prerequisite_buildings.begin(); prereq_it != prerequisite_buildings.end(); ++prereq_it){
			if(max_level < prereq_it->building_level){
				max_level = prereq_it->building_level;
			}
		}
		if(max_level < it->second){
			LOG_EMPERY_CENTER_DEBUG("Display prerequisite not met: tech_id = ", it->first,
				", level_required = ", it->second, ", max_level = ", max_level);
			return Response(Msg::ERR_DISPLAY_PREREQUISITE_NOT_MET) <<it->first;
		}
	}
	std::vector<ResourceTransactionElement> transaction;
	for(auto it = tech_data->upgrade_cost.begin(); it != tech_data->upgrade_cost.end(); ++it){
		transaction.emplace_back(ResourceTransactionElement::OP_REMOVE, it->first, it->second,
			ReasonIds::ID_UPGRADE_TECH, tech_data->tech_id_level.first.get(), tech_data->tech_id_level.second, 0);
	}
	const auto insuff_resource_id = castle->commit_resource_transaction_nothrow(transaction,
		[&]{ castle->create_tech_mission(tech_id, Castle::MIS_UPGRADE); });
	if(insuff_resource_id){
		return Response(Msg::ERR_CASTLE_NO_ENOUGH_RESOURCES) <<insuff_resource_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleCancelTechMission, account_uuid, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account_uuid){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	const auto tech_id = TechId(req.tech_id);
	castle->pump_tech_status(tech_id);

	const auto info = castle->get_tech(tech_id);
	if(info.mission == Castle::MIS_NONE){
		return Response(Msg::ERR_NO_TECH_MISSION) <<tech_id;
	}

	const auto tech_data = Data::CastleTech::require(info.tech_id, info.tech_level + 1);
	std::vector<ResourceTransactionElement> transaction;
	if((info.mission == Castle::MIS_CONSTRUCT) || (info.mission == Castle::MIS_UPGRADE)){
		const auto refund_ratio = Data::Global::as_double(Data::Global::SLOT_CASTLE_CANCELLATION_REFUND_RATIO);
		for(auto it = tech_data->upgrade_cost.begin(); it != tech_data->upgrade_cost.end(); ++it){
			transaction.emplace_back(ResourceTransactionElement::OP_ADD,
				it->first, static_cast<boost::uint64_t>(std::floor(it->second * refund_ratio + 0.001)),
				ReasonIds::ID_CANCEL_UPGRADE_TECH, info.tech_id.get(), info.tech_level, 0);
		}
	}
	castle->commit_resource_transaction(transaction,
		[&]{ castle->cancel_tech_mission(tech_id); });

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleCompleteTechImmediately, account_uuid, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account_uuid){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	const auto tech_id = TechId(req.tech_id);
	castle->pump_tech_status(tech_id);

	const auto info = castle->get_tech(tech_id);
	if(info.mission == Castle::MIS_NONE){
		return Response(Msg::ERR_NO_TECH_MISSION) <<tech_id;
	}

	const auto utc_now = Poseidon::get_utc_time();

	const auto item_box = ItemBoxMap::require(account_uuid);

	const auto trade_id = TradeId(Data::Global::as_unsigned(Data::Global::SLOT_CASTLE_TECH_IMMEDIATE_UPGRADE_TRADE_ID));
	const auto trade_data = Data::ItemTrade::require(trade_id);
	const auto time_remaining = saturated_sub(info.mission_time_end, utc_now);
	const auto trade_count = static_cast<boost::uint64_t>(std::ceil(time_remaining / 60000.0));
	std::vector<ItemTransactionElement> transaction;
	Data::unpack_item_trade(transaction, trade_data, trade_count, req.ID);
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction,
		[&]{ castle->speed_up_tech_mission(tech_id, UINT64_MAX); });
	if(insuff_item_id){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<insuff_item_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleQueryIndividualTechInfo, account_uuid, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account_uuid){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	const auto tech_id = TechId(req.tech_id);
	castle->pump_tech_status(tech_id);
	castle->synchronize_tech_with_player(tech_id, session);

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleQueryMyCastleList, account_uuid, session, /* req */){
	std::vector<boost::shared_ptr<MapObject>> map_objects;
	WorldMap::get_map_objects_by_owner(map_objects, account_uuid);
	for(auto it = map_objects.begin(); it != map_objects.end(); ++it){
		const auto &map_object = *it;
		if(map_object->get_map_object_type_id() != MapObjectTypeIds::ID_CASTLE){
			continue;
		}
		synchronize_map_object_with_player(map_object, session);
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleHarvestAllResources, account_uuid, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account_uuid){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	std::vector<boost::shared_ptr<MapCell>> map_cells;
	WorldMap::get_map_cells_by_parent_object(map_cells, map_object_uuid);
	if(map_cells.empty()){
		return Response(Msg::ERR_CASTLE_HAS_NO_MAP_CELL) <<map_object_uuid;
	}

	const auto max_amounts = castle->get_max_resource_amounts();

	bool harvested_some = false;
	for(auto it = map_cells.begin(); it != map_cells.end(); ++it){
		const auto &map_cell = *it;
		const auto resource_id = map_cell->get_production_resource_id();

		map_cell->pump_status();

		const auto rit = max_amounts.find(resource_id);
		if(rit == max_amounts.end()){
			LOG_EMPERY_CENTER_DEBUG("There is no warehouse? map_object_uuid = ", map_object_uuid, ", resource_id = ", resource_id);
			continue;
		}
		const auto max_amount = rit->second;
		const auto current_amount = castle->get_resource(resource_id).amount;
		const auto amount_harvested = map_cell->harvest(castle, saturated_sub(max_amount, current_amount));
		if(amount_harvested == 0){
			LOG_EMPERY_CENTER_DEBUG("No resource harvested: map_object_uuid = ", map_object_uuid,
				", coord = ", map_cell->get_coord(), ", resource_id = ", resource_id);
			continue;
		}

		harvested_some = true;
	}
	if(!harvested_some){
		return Response(Msg::ERR_WAREHOUSE_FULL);
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleQueryMapCells, account_uuid, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account_uuid){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	std::vector<boost::shared_ptr<MapCell>> map_cells;
	WorldMap::get_map_cells_by_parent_object(map_cells, map_object_uuid);
	if(map_cells.empty()){
		return Response(Msg::ERR_CASTLE_HAS_NO_MAP_CELL) <<map_object_uuid;
	}
	for(auto it = map_cells.begin(); it != map_cells.end(); ++it){
		const auto &map_cell = *it;
		map_cell->pump_status();
		map_cell->synchronize_with_player(session);
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleCreateImmigrants, account_uuid, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account_uuid){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	const auto castle_level = castle->get_level();
	const auto upgrade_data = Data::CastleUpgradePrimary::require(castle_level);
	if(upgrade_data->max_immigrant_group_count == 0){
		return Response(Msg::ERR_CASTLE_LEVEL_TOO_LOW);
	}

	std::size_t immigrant_group_count = 0;
	std::vector<boost::shared_ptr<MapObject>> child_objects;
	WorldMap::get_map_objects_by_parent_object(child_objects, map_object_uuid);
	for(auto it = child_objects.begin(); it != child_objects.end(); ++it){
		const auto &child = *it;
		const auto child_type_id = child->get_map_object_type_id();
		if((child_type_id != MapObjectTypeIds::ID_CASTLE) && (child_type_id != MapObjectTypeIds::ID_IMMIGRANTS)){
			continue;
		}
		LOG_EMPERY_CENTER_DEBUG("Found child castle or immigrant group: map_object_uuid = ", map_object_uuid,
			", child_object_uuid = ", child->get_map_object_uuid(), ", child_type_id = ", child_type_id);
		++immigrant_group_count;
	}
	if(immigrant_group_count >= upgrade_data->max_immigrant_group_count){
		return Response(Msg::ERR_MAX_NUMBER_OF_IMMIGRANT_GROUPS);
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
			LOG_EMPERY_CENTER_DEBUG("Found coord for immigrant: coord = ", coord);
			break;
		}
		foundation.erase(foundation.begin());
	}
	const auto &coord = foundation.front();

	const auto item_box = ItemBoxMap::require(account_uuid);

	std::vector<ItemTransactionElement> transaction;
	const auto trade_id = TradeId(Data::Global::as_unsigned(Data::Global::SLOT_IMMIGRANT_CREATION_TRADE_ID));
	const auto trade_data = Data::ItemTrade::require(trade_id);
	Data::unpack_item_trade(transaction, trade_data, 1, req.ID);
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction,
		[&]{
			const auto immigrants_uuid = MapObjectUuid(Poseidon::Uuid::random());
			const auto immigrants = boost::make_shared<MapObject>(immigrants_uuid, MapObjectTypeIds::ID_IMMIGRANTS,
				account_uuid, map_object_uuid, std::string(), coord);
			immigrants->pump_status();
			WorldMap::insert_map_object(immigrants);
			LOG_EMPERY_CENTER_INFO("Created immigrant group: immigrants_uuid = ", immigrants_uuid, ", account_uuid = ", account_uuid);
		});
	if(insuff_item_id){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<insuff_item_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleSpeedUpBuildingUpgrade, account_uuid, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account_uuid){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	const auto building_base_id = BuildingBaseId(req.building_base_id);
	castle->pump_building_status(building_base_id);

	const auto info = castle->get_building_base(building_base_id);
	if(info.mission == Castle::MIS_NONE){
		return Response(Msg::ERR_NO_BUILDING_MISSION) <<building_base_id;
	}

	const auto item_id = ItemId(req.item_id);
	const auto item_data = Data::Item::require(item_id);
	if(item_data->type.first != Data::Item::CAT_UPGRADE_TURBO){
		return Response(Msg::ERR_ITEM_TYPE_MISMATCH) <<(unsigned)Data::Item::CAT_UPGRADE_TURBO;
	}
	if((item_data->type.second != 1) && (item_data->type.second != 3)){
		return Response(Msg::ERR_NOT_BUILDING_UPGRADE_ITEM) <<item_id;
	}
	const auto turbo_milliseconds = saturated_mul(item_data->value, (boost::uint64_t)60000);

	const auto utc_now = Poseidon::get_utc_time();

	const auto item_box = ItemBoxMap::require(account_uuid);

	const auto time_remaining = saturated_sub(info.mission_time_end, utc_now);
	const auto count_to_consume = std::min<boost::uint64_t>(req.count,
		saturated_add(time_remaining, turbo_milliseconds - 1) / turbo_milliseconds);
	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(ItemTransactionElement::OP_REMOVE, item_id, count_to_consume,
		ReasonIds::ID_SPEED_UP_BUILDING_UPGRADE, info.building_id.get(), info.building_level, 0);
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction,
		[&]{ castle->speed_up_building_mission(building_base_id, saturated_mul(turbo_milliseconds, count_to_consume)); });
	if(insuff_item_id){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<insuff_item_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleSpeedUpTechUpgrade, account_uuid, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account_uuid){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	const auto tech_id = TechId(req.tech_id);
	castle->pump_tech_status(tech_id);

	const auto info = castle->get_tech(tech_id);
	if(info.mission == Castle::MIS_NONE){
		return Response(Msg::ERR_NO_TECH_MISSION) <<tech_id;
	}

	const auto item_id = ItemId(req.item_id);
	const auto item_data = Data::Item::require(item_id);
	if(item_data->type.first != Data::Item::CAT_UPGRADE_TURBO){
		return Response(Msg::ERR_ITEM_TYPE_MISMATCH) <<(unsigned)Data::Item::CAT_UPGRADE_TURBO;
	}
	if((item_data->type.second != 1) && (item_data->type.second != 4)){
		return Response(Msg::ERR_NOT_TECH_UPGRADE_ITEM) <<item_id;
	}
	const auto turbo_milliseconds = saturated_mul(item_data->value, (boost::uint64_t)60000);

	const auto utc_now = Poseidon::get_utc_time();

	const auto item_box = ItemBoxMap::require(account_uuid);

	const auto time_remaining = saturated_sub(info.mission_time_end, utc_now);
	const auto count_to_consume = std::min<boost::uint64_t>(req.count,
		saturated_add(time_remaining, turbo_milliseconds - 1) / turbo_milliseconds);
	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(ItemTransactionElement::OP_REMOVE, item_id, count_to_consume,
		ReasonIds::ID_SPEED_UP_TECH_UPGRADE, info.tech_id.get(), info.tech_level, 0);
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction,
		[&]{ castle->speed_up_tech_mission(tech_id, saturated_mul(turbo_milliseconds, count_to_consume)); });
	if(insuff_item_id){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<insuff_item_id;
	}

	return Response();
}

}
