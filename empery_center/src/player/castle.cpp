#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/cs_castle.hpp"
#include "../msg/err_castle.hpp"
#include "../msg/err_map.hpp"
#include "../resource_utilities.hpp"
#include "../singletons/world_map.hpp"
#include "../castle.hpp"
#include "../data/castle.hpp"
#include "../transaction_element.hpp"
#include "../reason_ids.hpp"
#include "../map_object_type_ids.hpp"
#include "../map_cell.hpp"
#include "../data/global.hpp"
#include "../data/vip.hpp"
#include "../singletons/item_box_map.hpp"
#include "../item_box.hpp"
#include "../data/item.hpp"
#include "../data/map_object_type.hpp"
#include "../msg/err_item.hpp"
#include "../map_utilities_center.hpp"
#include "../building_type_ids.hpp"
#include "../account_attribute_ids.hpp"
#include "../attribute_ids.hpp"
#include "../singletons/task_box_map.hpp"
#include "../task_box.hpp"
#include "../task_type_ids.hpp"
#include "../map_utilities.hpp"
#include "../announcement.hpp"
#include "../singletons/announcement_map.hpp"
#include "../chat_message_type_ids.hpp"
#include "../chat_message_slot_ids.hpp"
#include "../item_ids.hpp"
#include "../buff_ids.hpp"
#include "../cluster_session.hpp"
#include "../auction_center.hpp"
#include "../singletons/auction_center_map.hpp"
#include "../data/map.hpp"
#include "../resource_ids.hpp"
#include "../events/castle.hpp"
#include "../account_utilities.hpp"
#include "../legion_task_box.hpp"
#include "../singletons/legion_member_map.hpp"
#include "../singletons/legion_task_box_map.hpp"
#include "../legion_member.hpp"
#include "../legion_member_attribute_ids.hpp"
#include <poseidon/async_job.hpp>
#include "../legion_log.hpp"
#include "../singletons/activity_map.hpp"
#include "../activity.hpp"
#include "../legion.hpp"
#include "../msg/sc_legion.hpp"
#include "../singletons/legion_map.hpp"
#include "../singletons/castle_offline_upgrade_building_base_map.hpp"

namespace EmperyCenter {

PLAYER_SERVLET(Msg::CS_CastleQueryInfo, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	const auto auction_center = AuctionCenterMap::require(account->get_account_uuid());

	auction_center->pump_status();

	castle->pump_status();
	castle->synchronize_with_player(session);

	std::vector<boost::shared_ptr<MapObject>> child_objects;
	WorldMap::get_map_objects_by_parent_object(child_objects, map_object_uuid);
	for(auto it = child_objects.begin(); it != child_objects.end(); ++it){
		const auto &child = *it;
		LOG_EMPERY_CENTER_DEBUG("Child object: map_object_uuid = ", map_object_uuid,
			", child_object_uuid = ", child->get_map_object_uuid(), ", child_object_type_id = ", child->get_map_object_type_id());
		child->pump_status();
		child->synchronize_with_player(session);
	}
	
	//同步离线升级建筑基础信息
	CastleOfflineUpgradeBuildingBaseMap::Synchronize_with_player(castle->get_owner_uuid(),map_object_uuid,session);

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleCreateBuilding, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	const auto task_box = TaskBoxMap::require(account->get_account_uuid());

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
	std::vector<Castle::BuildingBaseInfo> old_infos;
	castle->get_buildings_by_id(old_infos, building_id);
	if(old_infos.size() >= building_data->build_limit){
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
		const auto max_level = castle->get_max_level(it->first);
		if(max_level < it->second){
			LOG_EMPERY_CENTER_DEBUG("Prerequisite not met: building_id = ", it->first,
				", level_required = ", it->second, ", max_level = ", max_level);
			return Response(Msg::ERR_PREREQUISITE_NOT_MET) <<it->first;
		}
	}
	const auto duration = static_cast<std::uint64_t>(std::ceil(upgrade_data->upgrade_duration * 60000.0 - 0.001));

	const auto insuff_resource_id = try_decrement_resources(castle, task_box, upgrade_data->upgrade_cost,
		ReasonIds::ID_UPGRADE_BUILDING, building_data->building_id.get(), upgrade_data->building_level, 0,
		[&]{ castle->create_building_mission(building_base_id, Castle::MIS_CONSTRUCT, duration, building_data->building_id); });
	if(insuff_resource_id){
		return Response(Msg::ERR_CASTLE_NO_ENOUGH_RESOURCES) <<insuff_resource_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleUpgradeBuilding, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	const auto task_box = TaskBoxMap::require(account->get_account_uuid());

	castle->pump_status();

	const auto queue_size = castle->get_building_queue_size();
	const auto max_queue_size = Data::Global::as_unsigned(Data::Global::SLOT_MAX_CONCURRENT_UPGRADING_BUILDING_COUNT);
	if(queue_size >= max_queue_size){
		return Response(Msg::ERR_BUILDING_QUEUE_FULL) <<max_queue_size;
	}

	const auto building_base_id = BuildingBaseId(req.building_base_id);
	// castle->pump_building_status(building_base_id);

	const auto info = castle->get_building_base(building_base_id);
	if(!info.building_id){
		return Response(Msg::ERR_NO_BUILDING_THERE) <<building_base_id;
	}
	if(info.mission != Castle::MIS_NONE){
		return Response(Msg::ERR_BUILDING_MISSION_CONFLICT) <<building_base_id;
	}

	const auto building_data = Data::CastleBuilding::require(info.building_id);
	switch(building_data->type.get()){
	case BuildingTypeIds::ID_ACADEMY.get():
		if(castle->is_tech_upgrade_in_progress()){
			return Response(Msg::ERR_TECH_UPGRADE_IN_PROGRESS);
		}
		break;
	case BuildingTypeIds::ID_STABLES.get():
	case BuildingTypeIds::ID_BARRACKS.get():
	case BuildingTypeIds::ID_ARCHER_BARRACKS.get():
	case BuildingTypeIds::ID_WEAPONRY.get():
		if(castle->is_soldier_production_in_progress(building_base_id)){
			return Response(Msg::ERR_BATTALION_PRODUCTION_IN_PROGRESS);
		}
		break;

	case BuildingTypeIds::ID_MEDICAL_TENT.get():
		if(castle->is_treatment_in_progress()){
			return Response(Msg::ERR_MEDICAL_TENT_TREATMENT_IN_PROGRESS);
		}
		break;
	}

	const auto new_level = info.building_level + 1;
	const auto noviciate_level_threshold = Data::Global::as_unsigned(Data::Global::SLOT_NOVICIATE_PROTECTION_CASTLE_LEVEL_THRESHOLD);
	if(new_level >= noviciate_level_threshold){
		async_cancel_noviciate_protection(castle->get_owner_uuid());
	}
	const auto upgrade_data = Data::CastleUpgradeAbstract::get(building_data->type, new_level);
	if(!upgrade_data){
		return Response(Msg::ERR_BUILDING_UPGRADE_MAX) <<info.building_id;
	}
	for(auto it = upgrade_data->prerequisite.begin(); it != upgrade_data->prerequisite.end(); ++it){
		const auto max_level = castle->get_max_level(it->first);
		if(max_level < it->second){
			LOG_EMPERY_CENTER_DEBUG("Prerequisite not met: building_id = ", it->first,
				", level_required = ", it->second, ", max_level = ", max_level);
			return Response(Msg::ERR_PREREQUISITE_NOT_MET) <<it->first;
		}
	}
	const auto duration = static_cast<std::uint64_t>(std::ceil(upgrade_data->upgrade_duration * 60000.0 - 0.001));

	const auto insuff_resource_id = try_decrement_resources(castle, task_box, upgrade_data->upgrade_cost,
		ReasonIds::ID_UPGRADE_BUILDING, building_data->building_id.get(), upgrade_data->building_level, 0,
		[&]{ castle->create_building_mission(building_base_id, Castle::MIS_UPGRADE, duration, BuildingId()); });
	if(insuff_resource_id){
		return Response(Msg::ERR_CASTLE_NO_ENOUGH_RESOURCES) <<insuff_resource_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleDestroyBuilding, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	castle->pump_status();
/*
	const auto queue_size = castle->get_building_queue_size();
	const auto max_queue_size = Data::Global::as_unsigned(Data::Global::SLOT_MAX_CONCURRENT_UPGRADING_BUILDING_COUNT);
	if(queue_size >= max_queue_size){
		return Response(Msg::ERR_BUILDING_QUEUE_FULL) <<max_queue_size;
	}
*/
	const auto building_base_id = BuildingBaseId(req.building_base_id);
	// castle->pump_building_status(building_base_id);

	const auto info = castle->get_building_base(building_base_id);
	if(!info.building_id){
		return Response(Msg::ERR_NO_BUILDING_THERE) <<building_base_id;
	}
	if(info.mission != Castle::MIS_NONE){
		return Response(Msg::ERR_BUILDING_MISSION_CONFLICT) <<building_base_id;
	}

	const auto building_base_data = Data::CastleBuildingBase::require(building_base_id);
	for(auto it = building_base_data->operation_prerequisite.begin(); it != building_base_data->operation_prerequisite.end(); ++it){
		const auto max_level = castle->get_max_level(it->first);
		if(max_level < it->second){
			LOG_EMPERY_CENTER_DEBUG("Operation prerequisite not met: building_id = ", it->first,
				", level_required = ", it->second, ", max_level = ", max_level);
			return Response(Msg::ERR_OPERATION_PREREQUISITE_NOT_MET) <<it->first;
		}
	}

	const auto building_data = Data::CastleBuilding::require(info.building_id);
	switch(building_data->type.get()){
	case BuildingTypeIds::ID_ACADEMY.get():
		if(castle->is_tech_upgrade_in_progress()){
			return Response(Msg::ERR_TECH_UPGRADE_IN_PROGRESS);
		}
		break;
	case BuildingTypeIds::ID_STABLES.get():
	case BuildingTypeIds::ID_BARRACKS.get():
	case BuildingTypeIds::ID_ARCHER_BARRACKS.get():
	case BuildingTypeIds::ID_WEAPONRY.get():
		if(castle->is_soldier_production_in_progress(building_base_id)){
			return Response(Msg::ERR_BATTALION_PRODUCTION_IN_PROGRESS);
		}
		break;

	case BuildingTypeIds::ID_MEDICAL_TENT.get():
		if(castle->has_wounded_soldiers()){
			return Response(Msg::ERR_MEDICAL_TENT_NOT_EMPTY);
		}
		break;
	}

	const auto upgrade_data = Data::CastleUpgradeAbstract::require(building_data->type, info.building_level);
	if(upgrade_data->destruct_duration == 0){
		return Response(Msg::ERR_BUILDING_NOT_DESTRUCTIBLE) <<info.building_id;
	}
	const auto duration = static_cast<std::uint64_t>(0 /* std::ceil(upgrade_data->destruct_duration * 60000.0 - 0.001) */);

	castle->create_building_mission(building_base_id, Castle::MIS_DESTRUCT, duration, BuildingId());

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleCompleteBuildingImmediately, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	const auto item_box = ItemBoxMap::require(account->get_account_uuid());

	const auto building_base_id = BuildingBaseId(req.building_base_id);
	castle->pump_building_status(building_base_id);

	const auto info = castle->get_building_base(building_base_id);
	if(info.mission == Castle::MIS_NONE){
		return Response(Msg::ERR_NO_BUILDING_MISSION) <<building_base_id;
	}

	const auto utc_now = Poseidon::get_utc_time();

	const auto trade_id = TradeId(Data::Global::as_unsigned(Data::Global::SLOT_CASTLE_IMMEDIATE_BUILDING_UPGRADE_TRADE_ID));
	const auto trade_data = Data::ItemTrade::require(trade_id);
	const auto time_remaining = saturated_sub(info.mission_time_end, utc_now);
	const auto trade_count = static_cast<std::uint64_t>(std::ceil(time_remaining / 60000.0 - 0.001));
	std::vector<ItemTransactionElement> transaction;
	Data::unpack_item_trade(transaction, trade_data, trade_count, req.ID);
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction, true,
		[&]{ castle->speed_up_building_mission(building_base_id, UINT64_MAX); });
	if(insuff_item_id){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<insuff_item_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleQueryIndividualBuildingInfo, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	const auto building_base_id = BuildingBaseId(req.building_base_id);
	castle->pump_building_status(building_base_id);
	castle->synchronize_building_with_player(building_base_id, session);

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleUpgradeTech, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	const auto task_box = TaskBoxMap::require(account->get_account_uuid());

	castle->pump_status();

	std::vector<Castle::BuildingBaseInfo> academy_infos;
	castle->get_buildings_by_type_id(academy_infos, BuildingTypeIds::ID_ACADEMY);
	if(academy_infos.empty()){
		return Response(Msg::ERR_NO_ACADEMY);
	}
	for(auto it = academy_infos.begin(); it != academy_infos.end(); ++it){
		if(it->mission != Castle::MIS_NONE){
			return Response(Msg::ERR_ACADEMY_UPGRADE_IN_PROGRESS);
		}
	}

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
	const auto duration = static_cast<std::uint64_t>(std::ceil(tech_data->upgrade_duration * 60000.0 - 0.001));

	for(auto it = tech_data->prerequisite.begin(); it != tech_data->prerequisite.end(); ++it){
		const auto max_level = castle->get_max_level(it->first);
		if(max_level < it->second){
			LOG_EMPERY_CENTER_DEBUG("Prerequisite not met: building_id = ", it->first,
				", level_required = ", it->second, ", max_level = ", max_level);
			return Response(Msg::ERR_PREREQUISITE_NOT_MET) <<it->first;
		}
	}
	for(auto it = tech_data->display_prerequisite.begin(); it != tech_data->display_prerequisite.end(); ++it){
		const auto max_level = castle->get_max_level(it->first);
		if(max_level < it->second){
			LOG_EMPERY_CENTER_DEBUG("Display prerequisite not met: building_id = ", it->first,
				", level_required = ", it->second, ", max_level = ", max_level);
			return Response(Msg::ERR_DISPLAY_PREREQUISITE_NOT_MET) <<it->first;
		}
	}
	for(auto it = tech_data->tech_prerequisite.begin(); it != tech_data->tech_prerequisite.end(); ++it){
		const auto tech_info = castle->get_tech(it->first);
		if(tech_info.tech_level < it->second){
			LOG_EMPERY_CENTER_DEBUG("Tech prerequisite not met: tech_id = ", it->first,
				", level_required = ", it->second, ", tech_level = ", tech_info.tech_level);
			return Response(Msg::ERR_TECH_PREREQUISITE_NOT_MET) <<it->first;
		}
	}

	const auto insuff_resource_id = try_decrement_resources(castle, task_box, tech_data->upgrade_cost,
		ReasonIds::ID_UPGRADE_TECH, tech_data->tech_id_level.first.get(), tech_data->tech_id_level.second, 0,
		[&]{ castle->create_tech_mission(tech_id, Castle::MIS_UPGRADE, duration); });
	if(insuff_resource_id){
		return Response(Msg::ERR_CASTLE_NO_ENOUGH_RESOURCES) <<insuff_resource_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleCompleteTechImmediately, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	const auto item_box = ItemBoxMap::require(account->get_account_uuid());

	const auto tech_id = TechId(req.tech_id);
	castle->pump_tech_status(tech_id);

	const auto info = castle->get_tech(tech_id);
	if(info.mission == Castle::MIS_NONE){
		return Response(Msg::ERR_NO_TECH_MISSION) <<tech_id;
	}

	const auto utc_now = Poseidon::get_utc_time();

	const auto trade_id = TradeId(Data::Global::as_unsigned(Data::Global::SLOT_CASTLE_IMMEDIATE_TECH_UPGRADE_TRADE_ID));
	const auto trade_data = Data::ItemTrade::require(trade_id);
	const auto time_remaining = saturated_sub(info.mission_time_end, utc_now);
	const auto trade_count = static_cast<std::uint64_t>(std::ceil(time_remaining / 60000.0 - 0.001));
	std::vector<ItemTransactionElement> transaction;
	Data::unpack_item_trade(transaction, trade_data, trade_count, req.ID);
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction, true,
		[&]{ castle->speed_up_tech_mission(tech_id, UINT64_MAX); });
	if(insuff_item_id){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<insuff_item_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleQueryIndividualTechInfo, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	const auto tech_id = TechId(req.tech_id);
	castle->pump_tech_status(tech_id);
	castle->synchronize_tech_with_player(tech_id, session);

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleQueryMyCastleList, account, session, /* req */){
	std::vector<boost::shared_ptr<MapObject>> map_objects;
	WorldMap::get_map_objects_by_owner(map_objects, account->get_account_uuid());
	for(auto it = map_objects.begin(); it != map_objects.end(); ++it){
		const auto &map_object = *it;
		if(map_object->get_map_object_type_id() != MapObjectTypeIds::ID_CASTLE){
			continue;
		}
		map_object->synchronize_with_player(session);
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleHarvestAllResources, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	const auto task_box = TaskBoxMap::require(account->get_account_uuid());

	const auto utc_now = Poseidon::get_utc_time();
	const auto harvest_cooldown = account->cast_attribute<std::uint64_t>(AccountAttributeIds::ID_CASTLE_HARVESTED_COOLDOWN);
	if(utc_now < harvest_cooldown){
		LOG_EMPERY_CENTER_DEBUG("In harvest cooldown: account_uuid = ", account->get_account_uuid());
		return Response();
	}
	boost::container::flat_map<AccountAttributeId, std::string> modifiers;
	modifiers[AccountAttributeIds::ID_CASTLE_HARVESTED_COOLDOWN] = boost::lexical_cast<std::string>(utc_now + 1000);
	account->set_attributes(std::move(modifiers));

	std::vector<boost::shared_ptr<MapCell>> map_cells;
	WorldMap::get_map_cells_by_parent_object(map_cells, map_object_uuid);
	WorldMap::get_map_cells_by_occupier_object(map_cells, map_object_uuid);
	if(map_cells.empty()){
		return Response(Msg::ERR_CASTLE_HAS_NO_MAP_CELL) <<map_object_uuid;
	}

	bool warehouse_full = true;
	for(auto it = map_cells.begin(); it != map_cells.end(); ++it){
		const auto &map_cell = *it;
		map_cell->pump_status();

		std::uint64_t amount_to_harvest = 0;
		const auto occupier_object_uuid = map_cell->get_occupier_object_uuid();
		if(occupier_object_uuid){
			LOG_EMPERY_CENTER_DEBUG("Map cell is occupied: map_object_uuid = ", map_object_uuid,
				", coord = ", map_cell->get_coord(), ", occupier_object_uuid = ", occupier_object_uuid);
		} else {
			amount_to_harvest = map_cell->get_resource_amount();
		}
		if(amount_to_harvest != 0){
			const auto resource_id = map_cell->get_production_resource_id();
			const auto amount_harvested = map_cell->harvest(castle, UINT32_MAX, false);
			if(amount_harvested == 0){
				LOG_EMPERY_CENTER_DEBUG("No resource harvested: map_object_uuid = ", map_object_uuid,
					", coord = ", map_cell->get_coord(), ", resource_id = ", resource_id);
				continue;
			}
			try {
				task_box->check(TaskTypeIds::ID_HARVEST_RESOURCES, resource_id.get(), amount_harvested,
					castle, 0, 0);
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			}
		}
		warehouse_full = false;
	}
	if(warehouse_full){
		return Response(Msg::ERR_WAREHOUSE_FULL);
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleQueryMapCells, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	std::vector<boost::shared_ptr<MapCell>> map_cells;
	WorldMap::get_map_cells_by_parent_object(map_cells, map_object_uuid);
	WorldMap::get_map_cells_by_occupier_object(map_cells, map_object_uuid);
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

PLAYER_SERVLET(Msg::CS_CastleCreateImmigrants, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	const auto item_box = ItemBoxMap::require(account->get_account_uuid());

	const auto castle_level = castle->get_level();
	const auto upgrade_data = Data::CastleUpgradePrimary::require(castle_level);
	if(upgrade_data->max_immigrant_group_count == 0){
		return Response(Msg::ERR_CASTLE_LEVEL_TOO_LOW);
	}

	std::size_t immigrant_group_count = 0;
	std::vector<boost::shared_ptr<MapObject>> temp_map_objects;
	WorldMap::get_map_objects_by_parent_object(temp_map_objects, map_object_uuid);
	for(auto it = temp_map_objects.begin(); it != temp_map_objects.end(); ++it){
		const auto &child_object = *it;
		const auto child_object_type_id = child_object->get_map_object_type_id();
		if((child_object_type_id != MapObjectTypeIds::ID_CASTLE) && (child_object_type_id != MapObjectTypeIds::ID_IMMIGRANTS)){
			continue;
		}
		LOG_EMPERY_CENTER_DEBUG("Found child castle or immigrant group: map_object_uuid = ", map_object_uuid,
			", child_object_uuid = ", child_object->get_map_object_uuid(), ", child_object_type_id = ", child_object_type_id);
		++immigrant_group_count;
	}
	if(immigrant_group_count >= upgrade_data->max_immigrant_group_count){
		return Response(Msg::ERR_MAX_NUMBER_OF_IMMIGRANT_GROUPS) <<upgrade_data->max_immigrant_group_count;
	}

	immigrant_group_count = 0;
	temp_map_objects.clear();
	WorldMap::get_map_objects_by_owner(temp_map_objects, account->get_account_uuid());
	for(auto it = temp_map_objects.begin(); it != temp_map_objects.end(); ++it){
		const auto &other_object = *it;
		const auto other_object_type_id = other_object->get_map_object_type_id();
		if((other_object_type_id != MapObjectTypeIds::ID_CASTLE) && (other_object_type_id != MapObjectTypeIds::ID_IMMIGRANTS)){
			continue;
		}
		LOG_EMPERY_CENTER_DEBUG("Found another castle or immigrant group: map_object_uuid = ", map_object_uuid,
			", other_object_uuid = ", other_object->get_map_object_uuid(), ", other_object_type_id = ", other_object_type_id);
		++immigrant_group_count;
	}
	const auto vip_data = Data::Vip::require(account->get_promotion_level());
	if(immigrant_group_count >= vip_data->max_castle_count + 1){
		return Response(Msg::ERR_ACCOUNT_MAX_IMMIGRANT_GROUPS) <<vip_data->max_castle_count;
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
			LOG_EMPERY_CENTER_DEBUG("Found coord for immigrants: coord = ", coord);
			break;
		}
		foundation.erase(foundation.begin());
	}
	const auto &coord = foundation.front();

	const auto immigrants_uuid = MapObjectUuid(Poseidon::Uuid::random());
	const auto utc_now = Poseidon::get_utc_time();

	std::vector<ItemTransactionElement> transaction;
	const auto trade_id = TradeId(Data::Global::as_unsigned(Data::Global::SLOT_IMMIGRANT_CREATION_TRADE_ID));
	const auto trade_data = Data::ItemTrade::require(trade_id);
	Data::unpack_item_trade(transaction, trade_data, 1, req.ID);
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction, true,
		[&]{
			const auto immigrants = boost::make_shared<MapObject>(immigrants_uuid, MapObjectTypeIds::ID_IMMIGRANTS,
				account->get_account_uuid(), map_object_uuid, std::string(), coord, utc_now, UINT64_MAX, false);
			immigrants->pump_status();
			WorldMap::insert_map_object(immigrants);
			LOG_EMPERY_CENTER_INFO("Created immigrant group: immigrants_uuid = ", immigrants_uuid,
				", account_uuid = ", account->get_account_uuid());
		});
	if(insuff_item_id){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<insuff_item_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleSpeedUpBuildingUpgrade, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	const auto item_box = ItemBoxMap::require(account->get_account_uuid());

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
	const auto turbo_milliseconds = saturated_mul<std::uint64_t>(item_data->value, 60000);

	const auto utc_now = Poseidon::get_utc_time();

	const auto time_remaining = saturated_sub(info.mission_time_end, utc_now);
	const auto count_to_consume = std::min<std::uint64_t>(req.count,
		saturated_add(time_remaining, turbo_milliseconds - 1) / turbo_milliseconds);
	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(ItemTransactionElement::OP_REMOVE, item_id, count_to_consume,
		ReasonIds::ID_SPEED_UP_BUILDING_UPGRADE, info.building_id.get(), info.building_level, 0);
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction, true,
		[&]{ castle->speed_up_building_mission(building_base_id, saturated_mul(turbo_milliseconds, count_to_consume)); });
	if(insuff_item_id){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<insuff_item_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleSpeedUpTechUpgrade, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	const auto item_box = ItemBoxMap::require(account->get_account_uuid());

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
	const auto turbo_milliseconds = saturated_mul<std::uint64_t>(item_data->value, 60000);

	const auto utc_now = Poseidon::get_utc_time();

	const auto time_remaining = saturated_sub(info.mission_time_end, utc_now);
	const auto count_to_consume = std::min<std::uint64_t>(req.count,
		saturated_add(time_remaining, turbo_milliseconds - 1) / turbo_milliseconds);
	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(ItemTransactionElement::OP_REMOVE, item_id, count_to_consume,
		ReasonIds::ID_SPEED_UP_TECH_UPGRADE, info.tech_id.get(), info.tech_level, 0);
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction, true,
		[&]{ castle->speed_up_tech_mission(tech_id, saturated_mul(turbo_milliseconds, count_to_consume)); });
	if(insuff_item_id){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<insuff_item_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleUseResourceBox, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	const auto item_box = ItemBoxMap::require(account->get_account_uuid());

	const auto item_id = ItemId(req.item_id);
	const auto item_data = Data::Item::require(item_id);
	if(item_data->type.first != Data::Item::CAT_RESOURCE_BOX){
		return Response(Msg::ERR_ITEM_TYPE_MISMATCH) <<(unsigned)Data::Item::CAT_RESOURCE_BOX;
	}
	const auto resource_id = ResourceId(item_data->type.second);
	const auto count_to_consume = req.repeat_count;
	const auto amount_to_add = checked_mul(count_to_consume, item_data->value);

	const auto map_object_uuid_head = Poseidon::load_be(reinterpret_cast<const std::uint64_t &>(map_object_uuid.get()[0]));

	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(ItemTransactionElement::OP_REMOVE, item_id, count_to_consume,
		ReasonIds::ID_UNPACK_INTO_CASTLE, map_object_uuid_head, item_id.get(), count_to_consume);
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction, false,
		[&]{
			std::vector<ResourceTransactionElement> res_transaction;
			res_transaction.emplace_back(ResourceTransactionElement::OP_ADD, resource_id, amount_to_add,
				ReasonIds::ID_UNPACK_INTO_CASTLE, map_object_uuid_head, item_id.get(), count_to_consume);
			castle->commit_resource_transaction(res_transaction);
		});
	if(insuff_item_id){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<insuff_item_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleUseResourceGift, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	const auto item_box = ItemBoxMap::require(account->get_account_uuid());

	const auto item_id = ItemId(req.item_id);
	const auto item_data = Data::Item::require(item_id);
	if(item_data->type.first != Data::Item::CAT_RESOURCE_GIFT_BOX){
		return Response(Msg::ERR_ITEM_TYPE_MISMATCH) <<(unsigned)Data::Item::CAT_RESOURCE_GIFT_BOX;
	}
	const auto trade_id = item_data->use_as_trade_id;
	if(!trade_id){
		return Response(Msg::ERR_ITEM_NOT_USABLE) <<item_id;
	}
	const auto trade_data = Data::ResourceTrade::require(trade_id);
	const auto count_to_consume = req.repeat_count;
	const auto map_object_uuid_head = Poseidon::load_be(reinterpret_cast<const std::uint64_t &>(map_object_uuid.get()[0]));

	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(ItemTransactionElement::OP_REMOVE, item_id, count_to_consume,
		ReasonIds::ID_UNPACK_INTO_CASTLE, map_object_uuid_head, item_id.get(), count_to_consume);
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction, false,
		[&]{
			std::vector<ResourceTransactionElement> res_transaction;
			for(auto it = trade_data->resource_produced.begin(); it != trade_data->resource_produced.end(); ++it){
				
				res_transaction.emplace_back(ResourceTransactionElement::OP_ADD, it->first, checked_mul(it->second, count_to_consume),
				ReasonIds::ID_UNPACK_INTO_CASTLE, map_object_uuid_head, item_id.get(), count_to_consume);
			}
			castle->commit_resource_transaction(res_transaction);
		});
	if(insuff_item_id){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<insuff_item_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleBeginSoldierProduction, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	const auto task_box = TaskBoxMap::require(account->get_account_uuid());

	const auto building_base_id = BuildingBaseId(req.building_base_id);

	const auto building_info = castle->get_building_base(building_base_id);
	if(!building_info.building_id){
		return Response(Msg::ERR_NO_BUILDING_THERE) <<building_base_id;
	}
	if(building_info.mission != Castle::MIS_NONE){
		return Response(Msg::ERR_BARRACKS_UPGRADE_IN_PROGRESS) <<building_base_id;
	}

	const auto info = castle->get_soldier_production(building_base_id);
	if(info.map_object_type_id){
		return Response(Msg::ERR_BATTALION_PRODUCTION_CONFLICT) <<info.map_object_type_id;
	}

	const auto map_object_type_id = MapObjectTypeId(req.map_object_type_id);
	const auto map_object_type_data = Data::MapObjectTypeBattalion::require(map_object_type_id);
	if(building_info.building_id != map_object_type_data->factory_id){
		return Response(Msg::ERR_FACTORY_ID_MISMATCH) <<map_object_type_data->factory_id;
	}
	const auto soldier_info = castle->get_soldier(map_object_type_id);
	if(!(soldier_info.enabled || map_object_type_data->enability_cost.empty())){
		return Response(Msg::ERR_BATTALION_UNAVAILABLE) <<map_object_type_id;
	}

	const auto count = req.count;
	if(count == 0){
		return Response(Msg::ERR_ZERO_SOLDIER_COUNT);
	}
	auto production_cost = map_object_type_data->production_cost;
	for(auto it = production_cost.begin(); it != production_cost.end(); ++it){
		it->second = checked_mul<std::uint64_t>(it->second, count);
	}
	const auto production_speed_turbo = castle->get_attribute(AttributeIds::ID_SOLDIER_PRODUCTION_SPEED_BONUS) / 1000.0;
	const auto duration = static_cast<std::uint64_t>(std::ceil(
		map_object_type_data->production_time * 60000.0 * count / (1 + production_speed_turbo) - 0.001));

	const auto insuff_resource_id = try_decrement_resources(castle, task_box, production_cost,
		ReasonIds::ID_PRODUCE_BATTALION, map_object_type_id.get(), count, 0,
		[&]{ castle->begin_soldier_production(building_base_id, map_object_type_id, count, duration); });
	if(insuff_resource_id){
		return Response(Msg::ERR_CASTLE_NO_ENOUGH_RESOURCES) <<insuff_resource_id;
	}

	try
	{
		//触发任务：累计建造部队，训练士兵，战力计算
		task_box->check(TaskTypeIds::ID_BUILD_SOLDIERS, TaskLegionPackageKeyIds::ID_BUILD_SOLDIERS.get(), (count)* map_object_type_data->warfare, TaskBox::TCC_ALL, 0, 0);
	}
	catch (std::exception &e)
	{
		LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
	}


	//军团任务兵种建造
	try{
		Poseidon::enqueue_async_job([=]{
			PROFILE_ME;
			if( map_object_type_data->warfare == 0){
					goto legion_build_battalion_done;
			}
			{
				const auto account_uuid = account->get_account_uuid();
				const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
				if(member){
					const auto legion_uuid = LegionUuid(member->get_legion_uuid());
					const auto legion_task_box = LegionTaskBoxMap::require(legion_uuid);
					legion_task_box->check(TaskTypeIds::ID_BUILD_SOLDIERS, TaskLegionKeyIds::ID_BUILD_SOLDIERS, (count)* map_object_type_data->warfare,account_uuid, 0, 0);
					legion_task_box->pump_status();
				}
			}
			legion_build_battalion_done:
			;
		});
	} catch (std::exception &e){
		LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleHarvestSoldier, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	const auto task_box = TaskBoxMap::require(account->get_account_uuid());

	const auto building_base_id = BuildingBaseId(req.building_base_id);

	const auto info = castle->get_soldier_production(building_base_id);
	if(!info.map_object_type_id){
		return Response(Msg::ERR_NO_BATTALION_PRODUCTION) <<building_base_id;
	}
	const auto utc_now = Poseidon::get_utc_time();
	if(utc_now < info.production_time_end){
		return Response(Msg::ERR_BATTALION_PRODUCTION_INCOMPLETE) <<building_base_id;
	}

	const auto count_harvested = castle->harvest_soldier(building_base_id);

	try {
		task_box->check(TaskTypeIds::ID_HARVEST_SOLDIERS, info.map_object_type_id.get(), count_harvested,
			castle, 0, 0);
	} catch(std::exception &e){
		LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
	}
	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleSpeedUpSoldierProduction, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	const auto item_box = ItemBoxMap::require(account->get_account_uuid());

	const auto building_base_id = BuildingBaseId(req.building_base_id);

	const auto info = castle->get_soldier_production(building_base_id);
	if(!info.map_object_type_id){
		return Response(Msg::ERR_NO_BATTALION_PRODUCTION) <<building_base_id;
	}

	const auto item_id = ItemId(req.item_id);
	const auto item_data = Data::Item::require(item_id);
	if(item_data->type.first != Data::Item::CAT_UPGRADE_TURBO){
		return Response(Msg::ERR_ITEM_TYPE_MISMATCH) <<(unsigned)Data::Item::CAT_UPGRADE_TURBO;
	}
	if((item_data->type.second != 1) && (item_data->type.second != 2)){
		return Response(Msg::ERR_NOT_BATTALION_PRODUCTION_ITEM) <<item_id;
	}
	const auto turbo_milliseconds = saturated_mul<std::uint64_t>(item_data->value, 60000);

	const auto utc_now = Poseidon::get_utc_time();

	const auto time_remaining = saturated_sub(info.production_time_end, utc_now);
	const auto count_to_consume = std::min<std::uint64_t>(req.count,
		saturated_add(time_remaining, turbo_milliseconds - 1) / turbo_milliseconds);
	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(ItemTransactionElement::OP_REMOVE, item_id, count_to_consume,
		ReasonIds::ID_SPEED_UP_BATTALION_PRODUCTION, info.map_object_type_id.get(), info.count, 0);
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction, true,
		[&]{ castle->speed_up_soldier_production(building_base_id, saturated_mul(turbo_milliseconds, count_to_consume)); });
	if(insuff_item_id){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<insuff_item_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleEnableSoldier, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	const auto task_box = TaskBoxMap::require(account->get_account_uuid());

	const auto map_object_type_id = MapObjectTypeId(req.map_object_type_id);
	const auto map_object_type_data = Data::MapObjectTypeBattalion::get(map_object_type_id);
	if(!map_object_type_data){
		return Response(Msg::ERR_NO_SUCH_MAP_OBJECT_TYPE) <<map_object_type_id;
	}
	const auto info = castle->get_soldier(map_object_type_id);
	if(info.enabled || map_object_type_data->enability_cost.empty()){
		return Response(Msg::ERR_BATTALION_UNLOCKED) <<map_object_type_id;
	}

	for(auto it = map_object_type_data->prerequisite.begin(); it != map_object_type_data->prerequisite.end(); ++it){
		const auto max_level = castle->get_max_level(it->first);
		if(max_level < it->second){
			LOG_EMPERY_CENTER_DEBUG("Prerequisite not met: building_id = ", it->first,
				", level_required = ", it->second, ", max_level = ", max_level);
			return Response(Msg::ERR_PREREQUISITE_NOT_MET) <<it->first;
		}
	}
	const auto previous_id = map_object_type_data->previous_id;
	if(previous_id){
		const auto prev_data = Data::MapObjectTypeBattalion::require(previous_id);
		const auto prev_info = castle->get_soldier(previous_id);
		if(!(prev_info.enabled || prev_data->enability_cost.empty())){
			return Response(Msg::ERR_PREREQUISITE_BATTALION_NOT_MET) <<previous_id;
		}
	}

	const auto insuff_resource_id = try_decrement_resources(castle, task_box, map_object_type_data->enability_cost,
		ReasonIds::ID_ENABLE_BATTALION, map_object_type_id.get(), 0, 0,
		[&]{ castle->enable_soldier(map_object_type_id); });
	if(insuff_resource_id){
		return Response(Msg::ERR_CASTLE_NO_ENOUGH_RESOURCES) <<insuff_resource_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleCreateBattalion, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	const auto map_object_type_id = MapObjectTypeId(req.map_object_type_id);
	const auto soldier_count = req.count;
	if(soldier_count < 1){
		return Response(Msg::ERR_ZERO_SOLDIER_COUNT);
	}

	const auto map_object_type_data = Data::MapObjectTypeBattalion::get(map_object_type_id);
	if(!map_object_type_data){
		return Response(Msg::ERR_NO_SUCH_MAP_OBJECT_TYPE) <<map_object_type_id;
	}
	const auto max_soldier_count_bonus = castle->get_max_soldier_count_bonus();
	const auto max_soldier_count = checked_add(map_object_type_data->max_soldier_count, max_soldier_count_bonus);
	if(soldier_count > max_soldier_count){
		return Response(Msg::ERR_TOO_MANY_SOLDIERS_FOR_BATTALION) <<max_soldier_count;
	}

	std::size_t battalion_count = 0;
	std::vector<boost::shared_ptr<MapObject>> current_battalions;
	WorldMap::get_map_objects_by_parent_object(current_battalions, map_object_uuid);
	for(auto it = current_battalions.begin(); it != current_battalions.end(); ++it){
		const auto defense_building = boost::dynamic_pointer_cast<DefenseBuilding>(*it);
		if(!defense_building){
			++battalion_count;
		}
	}
	const auto max_battalion_count = castle->get_max_battalion_count();
	if(battalion_count >= max_battalion_count){
		return Response(Msg::ERR_MAX_BATTALION_COUNT_EXCEEDED) <<max_battalion_count;
	}

	const auto battalion_uuid = MapObjectUuid(Poseidon::Uuid::random());
	const auto utc_now        = Poseidon::get_utc_time();

	const auto castle_uuid_head    = Poseidon::load_be(reinterpret_cast<const std::uint64_t &>(map_object_uuid.get()[0]));
	const auto battalion_uuid_head = Poseidon::load_be(reinterpret_cast<const std::uint64_t &>(battalion_uuid.get()[0]));

	const auto hp_total = checked_mul(soldier_count, map_object_type_data->hp_per_soldier);

	boost::container::flat_map<AttributeId, std::int64_t> modifiers;
	modifiers[AttributeIds::ID_SOLDIER_COUNT]     = static_cast<std::int64_t>(soldier_count);
	modifiers[AttributeIds::ID_SOLDIER_COUNT_MAX] = static_cast<std::int64_t>(soldier_count);
	modifiers[AttributeIds::ID_HP_TOTAL]          = static_cast<std::int64_t>(hp_total);

	std::vector<SoldierTransactionElement> transaction;
	transaction.emplace_back(SoldierTransactionElement::OP_REMOVE, map_object_type_id, soldier_count,
		ReasonIds::ID_CREATE_BATTALION, castle_uuid_head, battalion_uuid_head, 0);
	const auto insuff_soldier_id = castle->commit_soldier_transaction_nothrow(transaction,
		[&]{
			const auto battalion = boost::make_shared<MapObject>(battalion_uuid, map_object_type_id,
				account->get_account_uuid(), map_object_uuid, std::string(), castle->get_coord(), utc_now, UINT64_MAX, true);
			battalion->set_attributes(std::move(modifiers));
			battalion->pump_status();

			WorldMap::insert_map_object(battalion);
			LOG_EMPERY_CENTER_DEBUG("Created battalion: battalion_uuid = ", battalion_uuid,
				", account_uuid = ", account->get_account_uuid());
		});
	if(insuff_soldier_id){
		return Response(Msg::ERR_CASTLE_NO_ENOUGH_SOLDIERS) <<insuff_soldier_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleCompleteSoldierProductionImmediately, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	const auto item_box = ItemBoxMap::require(account->get_account_uuid());

	const auto building_base_id = BuildingBaseId(req.building_base_id);

	const auto info = castle->get_soldier_production(building_base_id);
	if(!info.map_object_type_id){
		return Response(Msg::ERR_NO_BATTALION_PRODUCTION) <<building_base_id;
	}

	const auto utc_now = Poseidon::get_utc_time();

	const auto trade_id = TradeId(Data::Global::as_unsigned(Data::Global::SLOT_CASTLE_IMMEDIATE_BATTALION_PRODUCTION_TRADE_ID));
	const auto trade_data = Data::ItemTrade::require(trade_id);
	const auto time_remaining = saturated_sub(info.production_time_end, utc_now);
	const auto trade_count = static_cast<std::uint64_t>(std::ceil(time_remaining / 60000.0 - 0.001));
	std::vector<ItemTransactionElement> transaction;
	Data::unpack_item_trade(transaction, trade_data, trade_count, req.ID);
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction, true,
		[&]{ castle->speed_up_soldier_production(building_base_id, UINT64_MAX); });
	if(insuff_item_id){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<insuff_item_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleCreateChildCastle, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	const auto item_box = ItemBoxMap::require(account->get_account_uuid());

	const auto coord = Coord(req.x, req.y);
	const auto test_cluster = WorldMap::get_cluster(coord);
	if(!test_cluster){
		return Response(Msg::ERR_CLUSTER_CONNECTION_LOST) <<coord;
	}

	const auto castle_level = castle->get_level();
	const auto upgrade_data = Data::CastleUpgradePrimary::require(castle_level);
	if(upgrade_data->max_immigrant_group_count == 0){
		return Response(Msg::ERR_CASTLE_LEVEL_TOO_LOW);
	}

	std::size_t immigrant_group_count = 0;
	std::vector<boost::shared_ptr<MapObject>> temp_map_objects;
	WorldMap::get_map_objects_by_parent_object(temp_map_objects, map_object_uuid);
	for(auto it = temp_map_objects.begin(); it != temp_map_objects.end(); ++it){
		const auto &child_object = *it;
		const auto child_object_type_id = child_object->get_map_object_type_id();
		if((child_object_type_id != MapObjectTypeIds::ID_CASTLE) && (child_object_type_id != MapObjectTypeIds::ID_IMMIGRANTS)){
			continue;
		}
		LOG_EMPERY_CENTER_DEBUG("Found child castle or immigrant group: map_object_uuid = ", map_object_uuid,
			", child_object_uuid = ", child_object->get_map_object_uuid(), ", child_object_type_id = ", child_object_type_id);
		++immigrant_group_count;
	}
	if(immigrant_group_count >= upgrade_data->max_immigrant_group_count){
		return Response(Msg::ERR_MAX_NUMBER_OF_IMMIGRANT_GROUPS) <<upgrade_data->max_immigrant_group_count;
	}

	immigrant_group_count = 0;
	temp_map_objects.clear();
	WorldMap::get_map_objects_by_owner(temp_map_objects, account->get_account_uuid());
	for(auto it = temp_map_objects.begin(); it != temp_map_objects.end(); ++it){
		const auto &other_object = *it;
		const auto other_object_type_id = other_object->get_map_object_type_id();
		if((other_object_type_id != MapObjectTypeIds::ID_CASTLE) && (other_object_type_id != MapObjectTypeIds::ID_IMMIGRANTS)){
			continue;
		}
		LOG_EMPERY_CENTER_DEBUG("Found another castle or immigrant group: map_object_uuid = ", map_object_uuid,
			", other_object_uuid = ", other_object->get_map_object_uuid(), ", other_object_type_id = ", other_object_type_id);
		++immigrant_group_count;
	}
	const auto vip_data = Data::Vip::require(account->get_promotion_level());
	if(immigrant_group_count >= vip_data->max_castle_count + 1){
		return Response(Msg::ERR_ACCOUNT_MAX_IMMIGRANT_GROUPS) <<vip_data->max_castle_count;
	}

	auto deploy_result = can_deploy_castle_at(coord, { }, false);
	if(deploy_result.first != Msg::ST_OK){
		return std::move(deploy_result);
	}

	const auto child_castle_uuid = MapObjectUuid(Poseidon::Uuid::random());
	const auto utc_now = Poseidon::get_utc_time();

	std::vector<ItemTransactionElement> transaction;
	const auto trade_id = TradeId(Data::Global::as_unsigned(Data::Global::SLOT_IMMIGRANT_CREATION_TRADE_ID));
	const auto trade_data = Data::ItemTrade::require(trade_id);
	Data::unpack_item_trade(transaction, trade_data, 1, req.ID);
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction, true,
		[&]{
			const auto child_castle = boost::make_shared<Castle>(child_castle_uuid,
				account->get_account_uuid(), map_object_uuid, std::move(req.name), coord, utc_now);
			child_castle->check_init_buildings();
			child_castle->check_init_resources();
			child_castle->pump_status();
			WorldMap::insert_map_object(child_castle);
			LOG_EMPERY_CENTER_INFO("Created castle: child_castle_uuid = ", child_castle_uuid,
				", owner_uuid = ", account->get_account_uuid(), ", coord = ", coord);
		});
	if(insuff_item_id){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<insuff_item_id;
	}

	try {
		const auto announcement_uuid = AnnouncementUuid(Poseidon::Uuid::random());
		const auto language_id       = LanguageId();

		std::vector<std::pair<ChatMessageSlotId, std::string>> segments;
		segments.reserve(4);
		segments.emplace_back(ChatMessageSlotIds::ID_NEW_CASTLE_OWNER,   account->get_account_uuid().str());
		segments.emplace_back(ChatMessageSlotIds::ID_NEW_CASTLE_COORD_X, boost::lexical_cast<std::string>(coord.x()));
		segments.emplace_back(ChatMessageSlotIds::ID_NEW_CASTLE_COORD_Y, boost::lexical_cast<std::string>(coord.y()));

		const auto announcement = boost::make_shared<Announcement>(announcement_uuid, language_id, utc_now,
			utc_now + 1000, 0, ChatMessageTypeIds::ID_NEW_CASTLE_NOTIFICATION, std::move(segments));
		AnnouncementMap::insert(announcement);
	} catch(std::exception &e){
		LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleDissolveBattalion, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	const auto map_object_type_id = MapObjectTypeId(req.map_object_type_id);
	const auto count = req.count;

	const auto map_object_type_data = Data::MapObjectTypeBattalion::get(map_object_type_id);
	if(!map_object_type_data){
		return Response(Msg::ERR_NO_SUCH_MAP_OBJECT_TYPE) <<map_object_type_id;
	}

	std::vector<SoldierTransactionElement> transaction;
	transaction.emplace_back(SoldierTransactionElement::OP_REMOVE, map_object_type_id, count,
		ReasonIds::ID_DISSOLVE_BATTALION, 0, 0, 0);
	const auto insuff_soldier_id = castle->commit_soldier_transaction_nothrow(transaction);
	if(insuff_soldier_id){
		return Response(Msg::ERR_CASTLE_NO_ENOUGH_SOLDIERS) <<insuff_soldier_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleUseSoldierBox, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	const auto item_box = ItemBoxMap::require(account->get_account_uuid());

	const auto item_id = ItemId(req.item_id);
	const auto item_data = Data::Item::require(item_id);
	if(item_data->type.first != Data::Item::CAT_SOLDIER_BOX){
		return Response(Msg::ERR_ITEM_TYPE_MISMATCH) <<(unsigned)Data::Item::CAT_SOLDIER_BOX;
	}
	const auto map_object_type_id = MapObjectTypeId(item_data->type.second);

	const auto map_object_uuid_head = Poseidon::load_be(reinterpret_cast<const std::uint64_t &>(map_object_uuid.get()[0]));

	const auto count_to_consume = req.repeat_count;
	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(ItemTransactionElement::OP_REMOVE, item_id, count_to_consume,
		ReasonIds::ID_UNPACK_INTO_CASTLE, map_object_uuid_head, item_id.get(), count_to_consume);
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction, false,
		[&]{
			std::vector<SoldierTransactionElement> res_transaction;
			res_transaction.emplace_back(SoldierTransactionElement::OP_ADD, map_object_type_id, checked_mul(count_to_consume, item_data->value),
				ReasonIds::ID_UNPACK_INTO_CASTLE, map_object_uuid_head, item_id.get(), count_to_consume);
			castle->commit_soldier_transaction(res_transaction);
		});
	if(insuff_item_id){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<insuff_item_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleBeginTreatment, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	const auto task_box = TaskBoxMap::require(account->get_account_uuid());

	castle->pump_status();

	std::vector<Castle::BuildingBaseInfo> medical_tent_infos;
	castle->get_buildings_by_type_id(medical_tent_infos, BuildingTypeIds::ID_MEDICAL_TENT);
	if(medical_tent_infos.empty()){
		return Response(Msg::ERR_NO_MEDICAL_TENT);
	}
	for(auto it = medical_tent_infos.begin(); it != medical_tent_infos.end(); ++it){
		if(it->mission != Castle::MIS_NONE){
			return Response(Msg::ERR_MEDICAL_TENT_UPGRADE_IN_PROGRESS);
		}
	}

	std::vector<Castle::TreatmentInfo> treatment_all;
	castle->get_treatment(treatment_all);
	if(!treatment_all.empty()){
		return Response(Msg::ERR_MEDICAL_TENT_BUSY);
	}

	double duration_minutes = 0;
	boost::container::flat_map<ResourceId, std::uint64_t> resources_consumed;

	boost::container::flat_map<MapObjectTypeId, std::uint64_t> soldiers;
	soldiers.reserve(req.soldiers.size());
	for(auto it = req.soldiers.begin(); it != req.soldiers.end(); ++it){
		const auto soldier_type_id = MapObjectTypeId(it->map_object_type_id);
		const auto wounded_info = castle->get_wounded_soldier(soldier_type_id);
		const auto count = std::min<std::uint64_t>(it->count, wounded_info.count);

		const auto soldier_type_data = Data::MapObjectTypeBattalion::require(soldier_type_id);
		duration_minutes += static_cast<double>(count) * soldier_type_data->healing_time;

		for(auto rit = soldier_type_data->healing_cost.begin(); rit != soldier_type_data->healing_cost.end(); ++rit){
			auto &amount_total = resources_consumed[ResourceId(rit->first)];
			amount_total = checked_add<std::uint64_t>(amount_total, rit->second);
		}

		auto &count_total = soldiers[MapObjectTypeId(it->map_object_type_id)];
		count_total = checked_add<std::uint64_t>(count_total, count);
	}
	const auto duration = static_cast<std::uint64_t>(duration_minutes * 60000);

	const auto insuff_resource_id = try_decrement_resources(castle, task_box, resources_consumed,
		ReasonIds::ID_BEGIN_SOLDIER_TREATMENT, 0, 0, 0,
		[&]{ castle->begin_treatment(soldiers, duration); });
	if(insuff_resource_id){
		return Response(Msg::ERR_CASTLE_NO_ENOUGH_RESOURCES) <<insuff_resource_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleSpeedUpTreatment, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	const auto item_box = ItemBoxMap::require(account->get_account_uuid());

	castle->pump_treatment();

	std::vector<Castle::TreatmentInfo> treatment_all;
	castle->get_treatment(treatment_all);
	if(treatment_all.empty()){
		return Response(Msg::ERR_NO_BATTALION_TREATMENT);
	}
	const auto treatment_time_end = treatment_all.front().time_end; // select any

	const auto item_id = ItemId(req.item_id);
	const auto item_data = Data::Item::require(item_id);
	if(item_data->type.first != Data::Item::CAT_UPGRADE_TURBO){
		return Response(Msg::ERR_ITEM_TYPE_MISMATCH) <<(unsigned)Data::Item::CAT_UPGRADE_TURBO;
	}
	if((item_data->type.second != 1) && (item_data->type.second != 5)){
		return Response(Msg::ERR_NOT_TREATMENT_ITEM) <<item_id;
	}
	const auto turbo_milliseconds = saturated_mul<std::uint64_t>(item_data->value, 60000);

	const auto utc_now = Poseidon::get_utc_time();

	const auto time_remaining = saturated_sub(treatment_time_end, utc_now);
	const auto count_to_consume = std::min<std::uint64_t>(req.count,
		saturated_add(time_remaining, turbo_milliseconds - 1) / turbo_milliseconds);
	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(ItemTransactionElement::OP_REMOVE, item_id, count_to_consume,
		ReasonIds::ID_SPEED_UP_SOLDIER_TREATMENT, 0, 0, 0);
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction, true,
		[&]{ castle->speed_up_treatment(saturated_mul(turbo_milliseconds, count_to_consume)); });
	if(insuff_item_id){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<insuff_item_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleCompleteTreatmentImmediately, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	const auto item_box = ItemBoxMap::require(account->get_account_uuid());

	castle->pump_treatment();

	std::vector<Castle::TreatmentInfo> treatment_all;
	castle->get_treatment(treatment_all);
	if(treatment_all.empty()){
		return Response(Msg::ERR_NO_BATTALION_TREATMENT);
	}
	const auto treatment_time_end = treatment_all.front().time_end; // select any

	const auto utc_now = Poseidon::get_utc_time();

	const auto trade_id = TradeId(Data::Global::as_unsigned(Data::Global::SLOT_CASTLE_IMMEDIATE_TREATMENT_TRADE_ID));
	const auto trade_data = Data::ItemTrade::require(trade_id);
	const auto time_remaining = saturated_sub(treatment_time_end, utc_now);
	const auto trade_count = static_cast<std::uint64_t>(std::ceil(time_remaining / 60000.0 - 0.001));
	std::vector<ItemTransactionElement> transaction;
	Data::unpack_item_trade(transaction, trade_data, trade_count, req.ID);
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction, true,
		[&]{ castle->speed_up_treatment(UINT64_MAX); });
	if(insuff_item_id){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<insuff_item_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleQueryTreatmentInfo, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	castle->pump_treatment();
	castle->synchronize_treatment_with_player(session);

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleRelocate, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	WorldMap::forced_reload_map_objects_by_parent_object(map_object_uuid);

	const auto item_box = ItemBoxMap::require(account->get_account_uuid());

	const auto relocation_card_count = item_box->get(ItemIds::ID_RELOCATION_CARD).count;
	if(relocation_card_count == 0){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<ItemIds::ID_RELOCATION_CARD;
	}

	const auto new_castle_coord = Coord(req.coord_x, req.coord_y);
	const auto new_cluster = WorldMap::get_cluster(new_castle_coord);
	if(!new_cluster){
		return Response(Msg::ERR_CLUSTER_CONNECTION_LOST) <<new_castle_coord;
	}
	const auto world_activity = ActivityMap::get_world_activity();
	if(world_activity && world_activity->is_on()){
		const auto old_cluster_coord = WorldMap::get_cluster_scope(castle->get_coord()).bottom_left();
		const auto new_cluster_coord = WorldMap::get_cluster_scope(new_castle_coord).bottom_left();
		if(old_cluster_coord != new_cluster_coord){
			return Response(Msg::ERR_CANNOT_DEPLOY_IN_WORLD_ACTIVITY);
		}
	}

	auto result = can_deploy_castle_at(new_castle_coord, map_object_uuid, false);
	if(result.first != Msg::ST_OK){
		return std::move(result);
	}

	if(castle->is_buff_in_effect(BuffIds::ID_BATTLE_STATUS)){
		return Response(Msg::ERR_BATTLE_IN_PROGRESS) <<map_object_uuid;
	}

	if(castle->is_buff_in_effect(BuffIds::ID_CASTLE_PROTECTION) && !castle->is_buff_in_effect(BuffIds::ID_CASTLE_PROTECTION_PREPARATION)){
		return Response(Msg::ERR_PROTECTION_IN_PROGRESS) <<map_object_uuid;
	}

	if(castle->is_buff_in_effect(BuffIds::ID_CASTLE_PROTECTION_PREPARATION)){
	   return Response(Msg::ERR_PROTECTION_PREPARATION_IN_PROGRESS) <<map_object_uuid;
	}

	std::vector<boost::shared_ptr<MapObject>> child_objects;
	WorldMap::get_map_objects_by_parent_object(child_objects, map_object_uuid);
	for(auto it = child_objects.begin(); it != child_objects.end(); ++it){
		const auto &child_object = *it;
		if(child_object->is_buff_in_effect(BuffIds::ID_BATTLE_STATUS)){
			return Response(Msg::ERR_BATTLE_IN_PROGRESS) <<child_object->get_map_object_uuid();
		}
	}

	std::vector<boost::shared_ptr<MapCell>> map_cells;
	WorldMap::get_map_cells_by_parent_object(map_cells, map_object_uuid);

	// 回收所有部队。
	for(auto it = child_objects.begin(); it != child_objects.end(); ++it){
		const auto &child_object = *it;
		const auto map_object_type_id = child_object->get_map_object_type_id();
		if(map_object_type_id == MapObjectTypeIds::ID_CASTLE){
			continue;
		}

		const auto child_object_data = Data::MapObjectTypeBattalion::get(map_object_type_id);
		if(child_object_data){
			child_object->pump_status();
			child_object->unload_resources(castle);
			child_object->set_coord(castle->get_coord());
			child_object->set_garrisoned(true);
		} else {
			child_object->delete_from_game();
		}
	}

	// 回收所有领地。
	for(auto it = map_cells.begin(); it != map_cells.end(); ++it){
		const auto &map_cell = *it;

		if(map_cell->is_buff_in_effect(BuffIds::ID_OCCUPATION_MAP_CELL)){
			continue;
		}

		map_cell->pump_status();
		map_cell->harvest(castle, UINT32_MAX, true);

		const auto ticket_item_id = map_cell->get_ticket_item_id();

		std::vector<ItemTransactionElement> transaction;
		transaction.emplace_back(ItemTransactionElement::OP_ADD, ticket_item_id, 1,
			ReasonIds::ID_RELOCATE_CASTLE, new_castle_coord.x(), new_castle_coord.y(), 0);
		if(map_cell->is_acceleration_card_applied()){
			transaction.emplace_back(ItemTransactionElement::OP_ADD, ItemIds::ID_ACCELERATION_CARD, 1,
				ReasonIds::ID_RELOCATE_CASTLE, new_castle_coord.x(), new_castle_coord.y(), 0);
		}
		item_box->commit_transaction(transaction, false,
			[&]{
				map_cell->set_parent_object({ }, { }, { });
				map_cell->set_acceleration_card_applied(false);
			});
	}

	// 迁城。
	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(ItemTransactionElement::OP_REMOVE, ItemIds::ID_RELOCATION_CARD, 1,
		ReasonIds::ID_RELOCATE_CASTLE, new_castle_coord.x(), new_castle_coord.y(), 0);
	item_box->commit_transaction(transaction, true,
		[&]{
			boost::container::flat_map<AttributeId, std::int64_t> modifiers;
			modifiers.reserve(8);
			modifiers[AttributeIds::ID_CASTLE_LAST_COORD_X] = castle->get_coord().x();
			modifiers[AttributeIds::ID_CASTLE_LAST_COORD_Y] = castle->get_coord().y();
			castle->set_attributes(std::move(modifiers));

			castle->set_coord(new_castle_coord);

			for(auto it = child_objects.begin(); it != child_objects.end(); ++it){
				const auto &child_object = *it;
				const auto map_object_type_id = child_object->get_map_object_type_id();
				if(map_object_type_id == MapObjectTypeIds::ID_CASTLE){
					continue;
				}
				child_object->set_coord(new_castle_coord);
			}
		});
	castle->pump_status();

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleActivateBuff, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	const auto item_box = ItemBoxMap::require(account->get_account_uuid());

	const auto item_id = ItemId(req.item_id);
	const auto item_data = Data::Item::require(item_id);
	if(item_data->type.first != Data::Item::CAT_CASTLE_BUFF){
		return Response(Msg::ERR_ITEM_TYPE_MISMATCH) <<(unsigned)Data::Item::CAT_CASTLE_BUFF;
	}
	const auto count = req.count;

	const auto buff_id = BuffId(item_data->type.second);
	const auto buff_duration = checked_mul(checked_mul<std::uint64_t>(item_data->value, 60000), count);

	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(ItemTransactionElement::OP_REMOVE, item_id, count,
		ReasonIds::ID_CASTLE_BUFF, buff_id.get(), 0, 0);
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction, true,
		[&]{ castle->accumulate_buff(buff_id, buff_duration); });
	if(insuff_item_id){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<insuff_item_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleReactivateCastle, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	if(!castle->is_garrisoned()){
		return Response(Msg::ERR_CASTLE_NOT_HUNG_UP) <<map_object_uuid;
	}

	WorldMap::forced_reload_map_objects_by_parent_object(map_object_uuid);

	std::vector<boost::shared_ptr<MapObject>> child_objects;
	WorldMap::get_map_objects_by_parent_object(child_objects, map_object_uuid);

	const auto new_castle_coord = Coord(req.coord_x, req.coord_y);
	const auto new_cluster = WorldMap::get_cluster(new_castle_coord);
	if(!new_cluster){
		return Response(Msg::ERR_CLUSTER_CONNECTION_LOST) <<new_castle_coord;
	}
	auto result = can_deploy_castle_at(new_castle_coord, map_object_uuid, false);
	if(result.first != Msg::ST_OK){
		return std::move(result);
	}
	castle->set_coord(new_castle_coord);
	castle->set_garrisoned(false);

	for(auto it = child_objects.begin(); it != child_objects.end(); ++it){
		const auto &child_object = *it;
		const auto map_object_type_id = child_object->get_map_object_type_id();
		if(map_object_type_id == MapObjectTypeIds::ID_CASTLE){
			continue;
		}
		child_object->set_coord(castle->get_coord());
	}

	castle->pump_status();

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleInitiateProtection, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	castle->pump_status();

	std::vector<ResourceTransactionElement> transaction;
	transaction.reserve(64);

	const auto days = req.days;
	const auto protection_duration = checked_mul<std::uint64_t>(days, 86400000);

	std::uint64_t delta_preparation_duration = 0;
	if(!castle->is_buff_in_effect(BuffIds::ID_CASTLE_PROTECTION)){
		const auto preparation_minutes = Data::Global::as_unsigned(Data::Global::SLOT_CASTLE_PROTECTION_PREPARATION_DURATION);
		delta_preparation_duration = checked_mul<std::uint64_t>(preparation_minutes, 60000);
	}
	const auto delta_protection_duration = checked_add(delta_preparation_duration, protection_duration);

	std::vector<boost::shared_ptr<MapObject>> map_objects;
	WorldMap::get_map_objects_by_parent_object(map_objects, map_object_uuid);
	map_objects.erase(
		std::remove_if(map_objects.begin(), map_objects.end(),
			[&](const boost::shared_ptr<MapObject> &child){ return child->get_map_object_type_id() == MapObjectTypeIds::ID_CASTLE; }),
		map_objects.end());
	map_objects.emplace_back(castle);

	std::vector<boost::shared_ptr<MapCell>> map_cells;
	WorldMap::get_map_cells_by_parent_object(map_cells, map_object_uuid);

	const auto map_object_uuid_head = Poseidon::load_be(reinterpret_cast<const std::uint64_t &>(map_object_uuid.get()[0]));

	const auto castle_level = castle->get_level();
	const auto upgrade_data = Data::CastleUpgradePrimary::require(castle_level);
	const auto &protection_cost = upgrade_data->protection_cost;

	for(auto it = protection_cost.begin(); it != protection_cost.end(); ++it){
		const auto amount_to_cost = checked_mul(it->second, days);
		transaction.emplace_back(ResourceTransactionElement::OP_REMOVE, it->first, amount_to_cost,
			ReasonIds::ID_CASTLE_PROTECTION, map_object_uuid_head, castle_level, protection_duration);
	}
	for(auto it = map_cells.begin(); it != map_cells.end(); ++it){
		const auto &map_cell = *it;
		if(!map_cell->is_protectable()){
			continue;
		}
		const auto coord = map_cell->get_coord();
		const auto cluster_scope = WorldMap::get_cluster_scope(coord);
		const auto basic_data = Data::MapCellBasic::require(static_cast<unsigned>(coord.x() - cluster_scope.left()),
		                                                    static_cast<unsigned>(coord.y() - cluster_scope.bottom()));
		const auto terrain_data = Data::MapTerrain::require(basic_data->terrain_id);
		const auto amount_to_cost = checked_mul(terrain_data->protection_cost, days);
		transaction.emplace_back(ResourceTransactionElement::OP_REMOVE, ResourceIds::ID_SPRING_WATER, amount_to_cost,
			ReasonIds::ID_CASTLE_PROTECTION, map_object_uuid_head, castle_level, protection_duration);
	}

	const auto insuff_resource_id = castle->commit_resource_transaction_nothrow(transaction,
		[&]{
			const auto utc_now = Poseidon::get_utc_time();
			for(auto it = map_objects.begin(); it != map_objects.end(); ++it){
				const auto &map_object = *it;
				map_object->clear_buff(BuffIds::ID_OCCUPATION_PROTECTION);
				map_object->accumulate_buff_hint(BuffIds::ID_CASTLE_PROTECTION_PREPARATION, utc_now, delta_preparation_duration);
				map_object->accumulate_buff_hint(BuffIds::ID_CASTLE_PROTECTION,             utc_now, delta_protection_duration);
			}
			for(auto it = map_cells.begin(); it != map_cells.end(); ++it){
				const auto &map_cell = *it;
				map_cell->clear_buff(BuffIds::ID_OCCUPATION_PROTECTION);
				map_cell->accumulate_buff_hint(BuffIds::ID_CASTLE_PROTECTION_PREPARATION, utc_now, delta_preparation_duration);
				map_cell->accumulate_buff_hint(BuffIds::ID_CASTLE_PROTECTION,             utc_now, delta_protection_duration);
			}
		});
	if(insuff_resource_id){
		return Response(Msg::ERR_CASTLE_NO_ENOUGH_RESOURCES) <<insuff_resource_id;
	}

	const auto protection_info = castle->get_buff(BuffIds::ID_CASTLE_PROTECTION);
	Poseidon::async_raise_event(
		boost::make_shared<Events::CastleProtection>(
			map_object_uuid, castle->get_owner_uuid(), delta_preparation_duration, delta_protection_duration, protection_info.time_end)
		);

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleCancelProtection, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	castle->pump_status();

	std::vector<ResourceTransactionElement> transaction;
	transaction.reserve(64);

	const auto protection_info = castle->get_buff(BuffIds::ID_CASTLE_PROTECTION);
	if(protection_info.duration == 0){
		return Response(Msg::ERR_CASTLE_NOT_UNDER_PROTECTION) <<map_object_uuid;
	}
	const auto preparation_info = castle->get_buff(BuffIds::ID_CASTLE_PROTECTION_PREPARATION);
	const auto protection_duration = saturated_sub(protection_info.duration, preparation_info.duration);
	const auto days = checked_add<std::uint64_t>(protection_duration, 86400000 - 1) / 86400000;

	std::vector<boost::shared_ptr<MapObject>> map_objects;
	WorldMap::get_map_objects_by_parent_object(map_objects, map_object_uuid);
	map_objects.erase(
		std::remove_if(map_objects.begin(), map_objects.end(),
			[&](const boost::shared_ptr<MapObject> &child){ return child->get_map_object_type_id() == MapObjectTypeIds::ID_CASTLE; }),
		map_objects.end());
	map_objects.emplace_back(castle);

	std::vector<boost::shared_ptr<MapCell>> map_cells;
	WorldMap::get_map_cells_by_parent_object(map_cells, map_object_uuid);

	if(castle->is_buff_in_effect(BuffIds::ID_CASTLE_PROTECTION_PREPARATION)){
		const auto refund_ratio = Data::Global::as_double(Data::Global::SLOT_CASTLE_PROTECTION_REFUND_RATIO);

		const auto map_object_uuid_head = Poseidon::load_be(reinterpret_cast<const std::uint64_t &>(map_object_uuid.get()[0]));

		const auto castle_level = castle->get_level();
		const auto upgrade_data = Data::CastleUpgradePrimary::require(castle_level);
		const auto &protection_cost = upgrade_data->protection_cost;

		for(auto it = protection_cost.begin(); it != protection_cost.end(); ++it){
			const auto amount_to_cost = checked_mul(it->second, days);
			const auto amount_to_refund = static_cast<std::uint64_t>(amount_to_cost * refund_ratio + 0.001);
			transaction.emplace_back(ResourceTransactionElement::OP_ADD, it->first, amount_to_refund,
				ReasonIds::ID_CASTLE_PROTECTION, map_object_uuid_head, castle_level, protection_duration);
		}
		for(auto it = map_cells.begin(); it != map_cells.end(); ++it){
			const auto &map_cell = *it;
			if(!map_cell->is_protectable()){
				continue;
			}
			const auto coord = map_cell->get_coord();
			const auto cluster_scope = WorldMap::get_cluster_scope(coord);
			const auto basic_data = Data::MapCellBasic::require(static_cast<unsigned>(coord.x() - cluster_scope.left()),
		                                                    	static_cast<unsigned>(coord.y() - cluster_scope.bottom()));
			const auto terrain_data = Data::MapTerrain::require(basic_data->terrain_id);
			const auto amount_to_cost = checked_mul(terrain_data->protection_cost, days);
			const auto amount_to_refund = static_cast<std::uint64_t>(amount_to_cost * refund_ratio + 0.001);
			transaction.emplace_back(ResourceTransactionElement::OP_ADD, ResourceIds::ID_SPRING_WATER, amount_to_refund,
				ReasonIds::ID_CASTLE_PROTECTION, map_object_uuid_head, castle_level, protection_duration);
		}
	}

	castle->commit_resource_transaction(transaction,
		[&]{
			for(auto it = map_objects.begin(); it != map_objects.end(); ++it){
				const auto &map_object = *it;
				map_object->clear_buff(BuffIds::ID_OCCUPATION_PROTECTION);
				map_object->clear_buff(BuffIds::ID_CASTLE_PROTECTION_PREPARATION);
				map_object->clear_buff(BuffIds::ID_CASTLE_PROTECTION);
			}
			for(auto it = map_cells.begin(); it != map_cells.end(); ++it){
				const auto &map_cell = *it;
				map_cell->clear_buff(BuffIds::ID_OCCUPATION_PROTECTION);
				map_cell->clear_buff(BuffIds::ID_CASTLE_PROTECTION_PREPARATION);
				map_cell->clear_buff(BuffIds::ID_CASTLE_PROTECTION);
			}
		});

	Poseidon::async_raise_event(
		boost::make_shared<Events::CastleProtection>(
			map_object_uuid, castle->get_owner_uuid(), 0, 0, 0)
		);

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleReactivateCastleRandom, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	if(!castle->is_garrisoned()){
		return Response(Msg::ERR_CASTLE_NOT_HUNG_UP) <<map_object_uuid;
	}

	WorldMap::forced_reload_map_objects_by_parent_object(map_object_uuid);

	std::vector<boost::shared_ptr<MapObject>> child_objects;
	WorldMap::get_map_objects_by_parent_object(child_objects, map_object_uuid);

	{
		const auto last_castle_coord = Coord(castle->get_attribute(AttributeIds::ID_CASTLE_LAST_COORD_X),
		                                     castle->get_attribute(AttributeIds::ID_CASTLE_LAST_COORD_Y));
		const auto last_cluster = WorldMap::get_cluster(last_castle_coord);
		if(last_cluster){
			auto result = can_deploy_castle_at(last_castle_coord, map_object_uuid, false);
			if(result.first == Msg::ST_OK){
				castle->set_coord(last_castle_coord);
				goto _reactivated;
			}
		}

		std::vector<std::pair<Coord, boost::shared_ptr<ClusterSession>>> clusters;
		WorldMap::get_clusters_all(clusters);

		boost::container::flat_multimap<std::uint64_t, Coord> cluster_coords_by_time;
		cluster_coords_by_time.reserve(clusters.size());
		for(auto it = clusters.begin(); it != clusters.end(); ++it){
			cluster_coords_by_time.emplace(it->second->get_created_time(), it->first);
		}

		std::deque<Coord> cluster_coords_avail;
		const auto castle_created_time = castle->get_created_time();
		auto it = cluster_coords_by_time.begin();
		for(; (it != cluster_coords_by_time.end()) && (it->first <= castle_created_time); ++it){
			cluster_coords_avail.emplace_front(it->second);
		}
		cluster_coords_avail.emplace_front(last_castle_coord);
		for(; it != cluster_coords_by_time.end(); ++it){
			cluster_coords_avail.emplace_back(it->second);
		}

		while(!cluster_coords_avail.empty()){
			const auto new_castle = WorldMap::place_castle_random_restricted(
				[&](Coord coord){
					castle->set_coord(coord);
					return castle;
				},
				cluster_coords_avail.front());
			if(new_castle == castle){
				goto _reactivated;
			}
			cluster_coords_avail.pop_front();
		}

		return Response(Msg::ERR_NO_START_POINTS_AVAILABLE);
	}
_reactivated:
	;
	castle->set_garrisoned(false);

	for(auto it = child_objects.begin(); it != child_objects.end(); ++it){
		const auto &child_object = *it;
		const auto map_object_type_id = child_object->get_map_object_type_id();
		if(map_object_type_id == MapObjectTypeIds::ID_CASTLE){
			continue;
		}
		child_object->set_coord(castle->get_coord());
	}

	castle->pump_status();

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleSetName, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	auto &name = req.name;
	constexpr std::size_t MAX_NAME_LEN = 31;
	if(name.size() > MAX_NAME_LEN){
		return Response(Msg::ERR_NICK_TOO_LONG) <<MAX_NAME_LEN;
	}

	const auto item_box = ItemBoxMap::require(account->get_account_uuid());

	std::vector<ItemTransactionElement> transaction;
	boost::container::flat_map<AccountAttributeId, std::string> modifiers;
	const auto first_castle_name_set = account->cast_attribute<bool>(AccountAttributeIds::ID_FIRST_CASTLE_NAME_SET);
	if(first_castle_name_set){
		const auto trade_id = TradeId(Data::Global::as_unsigned(Data::Global::SLOT_CASTLE_NAME_MODIFICATION_TRADE_ID));
		const auto trade_data = Data::ItemTrade::require(trade_id);
		Data::unpack_item_trade(transaction, trade_data, 1, req.ID);
	} else {
		modifiers[AccountAttributeIds::ID_FIRST_CASTLE_NAME_SET] = "1";
	}
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction, true,
		[&]{
			castle->set_name(std::move(name));
			account->set_attributes(std::move(modifiers));
		});
	if(insuff_item_id){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<insuff_item_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleUnlockTechEra, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

//	const auto task_box = TaskBoxMap::require(account->get_account_uuid());

	const auto tech_era = static_cast<unsigned>(req.tech_era);
	std::vector<boost::shared_ptr<const Data::CastleTech>> techs_in_era;
	Data::CastleTech::get_by_era(techs_in_era, tech_era);
	if(techs_in_era.empty()){
		return Response(Msg::ERR_TECH_ERA_NOT_FOUND) <<tech_era;
	}
	const auto info = castle->get_tech_era(tech_era);
	if(info.unlocked){
		return Response(Msg::ERR_TECH_ERA_UNLOCKED) <<tech_era;
	}

	castle->unlock_tech_era(tech_era);

	return Response();
}

PLAYER_SERVLET(Msg::CS_UsePersonalDoateItem, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	const auto item_box = ItemBoxMap::require(account->get_account_uuid());

	const auto item_id = ItemId(req.item_id);
	const auto item_data = Data::Item::require(item_id);
	if(item_data->type.first != Data::Item::CAT_PERSONAL_DONATE_BOX){
		return Response(Msg::ERR_ITEM_TYPE_MISMATCH) <<(unsigned)Data::Item::CAT_PERSONAL_DONATE_BOX;
	}
	const auto count_to_consume = req.repeat_count;
	const auto amount_to_add = checked_mul(count_to_consume, item_data->value);

	const auto map_object_uuid_head = Poseidon::load_be(reinterpret_cast<const std::uint64_t &>(map_object_uuid.get()[0]));

	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(ItemTransactionElement::OP_REMOVE, item_id, count_to_consume,
		ReasonIds::ID_UNPACK_INTO_CASTLE, map_object_uuid_head, item_id.get(), count_to_consume);
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction, false,
		[&]{
			const auto member = LegionMemberMap::get_by_account_uuid(account->get_account_uuid());
			if(member){
				boost::container::flat_map<LegionMemberAttributeId, std::string> legion_attributes_modifer;
				std::string donate = member->get_attribute(LegionMemberAttributeIds::ID_DONATE);
				if(donate.empty()){
					legion_attributes_modifer[LegionMemberAttributeIds::ID_DONATE] = boost::lexical_cast<std::string>(amount_to_add);
					LegionLog::LegionPersonalDonateTrace(account->get_account_uuid(),0,amount_to_add,ReasonIds::ID_LEGION_USE_DONATE_ITEM,item_id.get(),count_to_consume,0);
				}else{
					legion_attributes_modifer[LegionMemberAttributeIds::ID_DONATE] = boost::lexical_cast<std::string>(boost::lexical_cast<uint64_t>(donate) + amount_to_add);
					LegionLog::LegionPersonalDonateTrace(account->get_account_uuid(),boost::lexical_cast<uint64_t>(donate),boost::lexical_cast<uint64_t>(donate) + amount_to_add,ReasonIds::ID_LEGION_USE_DONATE_ITEM,item_id.get(),count_to_consume,0);
				}
				member->set_attributes(std::move(legion_attributes_modifer));
				auto legion = LegionMap::require(member->get_legion_uuid());
				// 广播通知
				Msg::SC_LegionNoticeMsg msg;
				msg.msgtype = Legion::LEGION_NOTICE_MSG_TYPE::LEGION_NOTICE_MSG_TYPE_USE_PERSONAL_DONATE_ITEM;
				msg.nick = "";
				msg.ext1 = "";
				legion->sendNoticeMsg(msg);
			}else{
				std::string donate = account->get_attribute(AccountAttributeIds::ID_DONATE);
				boost::container::flat_map<AccountAttributeId, std::string> account_attributes_modifer;
				if(donate.empty()){
					account_attributes_modifer[AccountAttributeIds::ID_DONATE] = boost::lexical_cast<std::string>(amount_to_add);
					LegionLog::LegionPersonalDonateTrace(account->get_account_uuid(),0,amount_to_add,ReasonIds::ID_LEGION_USE_DONATE_ITEM,item_id.get(),count_to_consume,0);
				}else{
					account_attributes_modifer[AccountAttributeIds::ID_DONATE] = boost::lexical_cast<std::string>(boost::lexical_cast<uint64_t>(donate) + amount_to_add);
					LegionLog::LegionPersonalDonateTrace(account->get_account_uuid(),boost::lexical_cast<uint64_t>(donate),boost::lexical_cast<uint64_t>(donate) + amount_to_add,ReasonIds::ID_LEGION_USE_DONATE_ITEM,item_id.get(),count_to_consume,0);	
				}
				account->set_attributes(std::move(account_attributes_modifer));
			}
		});
	if(insuff_item_id){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<insuff_item_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleResourceBattalionUnloadReset, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}
	castle->reset_resource_battalion_unload();
	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleNewGuideCreateSolider, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}
	const auto map_object_type_id = MapObjectTypeId(req.map_object_type_id);
	const auto map_object_type_data = Data::MapObjectTypeBattalion::get(map_object_type_id);
	if(!map_object_type_data){
		return Response(Msg::ERR_NO_SUCH_MAP_OBJECT_TYPE) <<map_object_type_id;
	}
	const auto count = req.count;
	std::vector<SoldierTransactionElement> res_transaction;
	res_transaction.emplace_back(SoldierTransactionElement::OP_ADD, map_object_type_id, count,
		ReasonIds::ID_NEW_GUIDE_CREATE_SOLIDER,map_object_type_id.get(), count, 0);
	castle->commit_soldier_transaction(res_transaction);
	return Response();
}

}
