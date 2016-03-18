#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/cs_castle.hpp"
#include "../msg/err_castle.hpp"
#include "../msg/err_map.hpp"
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
#include "../castle_utilities.hpp"
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

	return Response();
}

namespace {
	template<typename CallbackT>
	std::pair<ResourceId, ItemId> try_decrement_resources(const boost::shared_ptr<Castle> &castle,
		const boost::shared_ptr<ItemBox> &item_box, const boost::shared_ptr<TaskBox> &task_box,
		const boost::container::flat_map<ResourceId, std::uint64_t> &resources_to_consume,
		const boost::container::flat_map<ItemId, std::uint64_t> &tokens_to_consume,
		ReasonId reason, std::uint64_t param1, std::uint64_t param2, std::uint64_t param3, CallbackT &&callback)
	{
		PROFILE_ME;

		boost::container::flat_map<ResourceId, std::uint64_t> resources_consumed;
		resources_consumed = resources_to_consume; // resources_consumed.reserve(resources_to_consume.size());
		boost::container::flat_map<ItemId, std::uint64_t> tokens_consumed;
		tokens_consumed.reserve(tokens_to_consume.size());

		std::vector<ResourceTransactionElement> resource_transaction;
		resource_transaction.reserve(resources_to_consume.size());
		std::vector<ItemTransactionElement> item_transaction;
		item_transaction.reserve(tokens_to_consume.size());

		for(auto it = tokens_to_consume.begin(); it != tokens_to_consume.end(); ++it){
			const auto item_id = it->first;
			const auto item_data = Data::Item::require(item_id);
			if(item_data->type.first != Data::Item::CAT_RESOURCE_TOKEN){
				LOG_EMPERY_CENTER_WARNING("The specified item is not a resource token: item_id = ", item_id);
				DEBUG_THROW(Exception, sslit("The specified item is not a resource token"));
			}
			const auto resource_id = ResourceId(item_data->type.second);
			const auto cit = resources_consumed.find(resource_id);
			if(cit == resources_consumed.end()){
				LOG_EMPERY_CENTER_DEBUG("$$ Unneeded token: item_id = ", item_id);
				continue;
			}
			const auto max_count_needed = checked_add(cit->second, item_data->value - 1) / item_data->value;
			LOG_EMPERY_CENTER_DEBUG("$$ Use token: item_id = ", item_id, ", count = ", it->second, ", max_count_needed = ", max_count_needed);
			const auto count_consumed = std::min(it->second, max_count_needed);
			cit->second = saturated_sub(cit->second, item_data->value * count_consumed);
			tokens_consumed.emplace(item_id, count_consumed);
		}

		for(auto it = resources_consumed.begin(); it != resources_consumed.end(); ++it){
			resource_transaction.emplace_back(ResourceTransactionElement::OP_REMOVE, it->first, it->second,
				reason, param1, param2, param3);
		}
		for(auto it = tokens_consumed.begin(); it != tokens_consumed.end(); ++it){
			item_transaction.emplace_back(ItemTransactionElement::OP_REMOVE, it->first, it->second,
				reason, param1, param2, param3);
		}

		try {
			const auto insuff_resource_id = castle->commit_resource_transaction_nothrow(resource_transaction,
				[&]{
					const auto insuff_item_id = item_box->commit_transaction_nothrow(item_transaction, true,
						[&]{
							std::forward<CallbackT>(callback)();
						});
					if(insuff_item_id){
						throw insuff_item_id;
					}
			});
			if(insuff_resource_id){
				throw insuff_resource_id;
			}
		} catch(ResourceId &insuff_resource_id){
			return std::make_pair(insuff_resource_id, ItemId());
		} catch(ItemId &insuff_item_id){
			return std::make_pair(ResourceId(), insuff_item_id);
		}

		try {
			for(auto it = resources_to_consume.begin(); it != resources_to_consume.end(); ++it){
				if(it->second == 0){
					continue;
				}
				task_box->check(TaskTypeIds::ID_CONSUME_RESOURCES, it->first.get(), it->second,
					castle, 0, 0);
			}
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
		}

		return { };
	}
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

	const auto item_box = ItemBoxMap::require(account->get_account_uuid());
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

	boost::container::flat_map<ItemId, std::uint64_t> tokens;
	tokens.reserve(req.tokens.size());
	for(auto it = req.tokens.begin(); it != req.tokens.end(); ++it){
		auto &count_total = tokens[ItemId(it->item_id)];
		count_total = checked_add<std::uint64_t>(count_total, it->count);
	}

	const auto result = try_decrement_resources(castle, item_box, task_box, upgrade_data->upgrade_cost, tokens,
		ReasonIds::ID_UPGRADE_BUILDING, building_data->building_id.get(), upgrade_data->building_level, 0,
		[&]{ castle->create_building_mission(building_base_id, Castle::MIS_CONSTRUCT, building_data->building_id); });
	if(result.first){
		return Response(Msg::ERR_CASTLE_NO_ENOUGH_RESOURCES) <<result.first;
	}
	if(result.second){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<result.second;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleCancelBuildingMission, account, session, req){
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

	const auto info = castle->get_building_base(building_base_id);
	if(info.mission == Castle::MIS_NONE){
		return Response(Msg::ERR_NO_BUILDING_MISSION) <<building_base_id;
	}

	const auto building_data = Data::CastleBuilding::require(info.building_id);
	const auto upgrade_data = Data::CastleUpgradeAbstract::require(building_data->type, info.building_level + 1);
	std::vector<ResourceTransactionElement> transaction;
	if((info.mission == Castle::MIS_CONSTRUCT) || (info.mission == Castle::MIS_UPGRADE)){
		const auto refund_ratio = Data::Global::as_double(Data::Global::SLOT_CASTLE_UPGRADE_CANCELLATION_REFUND_RATIO);
		for(auto it = upgrade_data->upgrade_cost.begin(); it != upgrade_data->upgrade_cost.end(); ++it){
			transaction.emplace_back(ResourceTransactionElement::OP_ADD,
				it->first, static_cast<std::uint64_t>(std::floor(it->second * refund_ratio + 0.001)),
				ReasonIds::ID_CANCEL_UPGRADE_BUILDING, info.building_id.get(), info.building_level, 0);
		}
	}
	castle->commit_resource_transaction(transaction,
		[&]{ castle->cancel_building_mission(building_base_id); });

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

	const auto item_box = ItemBoxMap::require(account->get_account_uuid());
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
	if(building_data->type == BuildingTypeIds::ID_ACADEMY){
		if(castle->is_tech_upgrade_in_progress()){
			return Response(Msg::ERR_TECH_UPGRADE_IN_PROGRESS);
		}
	} else if((building_data->type == BuildingTypeIds::ID_STABLES) || (building_data->type == BuildingTypeIds::ID_BARRACKS) ||
		(building_data->type == BuildingTypeIds::ID_ARCHER_BARRACKS) || (building_data->type == BuildingTypeIds::ID_WEAPONRY))
	{
		if(castle->is_battalion_production_in_progress(building_base_id)){
			return Response(Msg::ERR_BATTALION_PRODUCTION_IN_PROGRESS);
		}
	}

	const auto upgrade_data = Data::CastleUpgradeAbstract::get(building_data->type, info.building_level + 1);
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

	boost::container::flat_map<ItemId, std::uint64_t> tokens;
	tokens.reserve(req.tokens.size());
	for(auto it = req.tokens.begin(); it != req.tokens.end(); ++it){
		auto &count_total = tokens[ItemId(it->item_id)];
		count_total = checked_add<std::uint64_t>(count_total, it->count);
	}

	const auto result = try_decrement_resources(castle, item_box, task_box, upgrade_data->upgrade_cost, tokens,
		ReasonIds::ID_UPGRADE_BUILDING, building_data->building_id.get(), upgrade_data->building_level, 0,
		[&]{ castle->create_building_mission(building_base_id, Castle::MIS_UPGRADE, { }); });
	if(result.first){
		return Response(Msg::ERR_CASTLE_NO_ENOUGH_RESOURCES) <<result.first;
	}
	if(result.second){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<result.second;
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
	if(building_data->type == BuildingTypeIds::ID_ACADEMY){
		if(castle->is_tech_upgrade_in_progress()){
			return Response(Msg::ERR_TECH_UPGRADE_IN_PROGRESS);
		}
	} else if((building_data->type == BuildingTypeIds::ID_STABLES) || (building_data->type == BuildingTypeIds::ID_BARRACKS) ||
		(building_data->type == BuildingTypeIds::ID_ARCHER_BARRACKS) || (building_data->type == BuildingTypeIds::ID_WEAPONRY))
	{
		if(castle->is_battalion_production_in_progress(building_base_id)){
			return Response(Msg::ERR_BATTALION_PRODUCTION_IN_PROGRESS);
		}
	}

	const auto upgrade_data = Data::CastleUpgradeAbstract::require(building_data->type, info.building_level);
	if(upgrade_data->destruct_duration == 0){
		return Response(Msg::ERR_BUILDING_NOT_DESTRUCTIBLE) <<info.building_id;
	}

	castle->create_building_mission(building_base_id, Castle::MIS_DESTRUCT, info.building_id);

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

	const auto item_box = ItemBoxMap::require(account->get_account_uuid());
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

	for(auto it = tech_data->prerequisite.begin(); it != tech_data->prerequisite.end(); ++it){
		const auto max_level = castle->get_max_level(it->first);
		if(max_level < it->second){
			LOG_EMPERY_CENTER_DEBUG("Prerequisite not met: tech_id = ", it->first,
				", level_required = ", it->second, ", max_level = ", max_level);
			return Response(Msg::ERR_PREREQUISITE_NOT_MET) <<it->first;
		}
	}
	for(auto it = tech_data->display_prerequisite.begin(); it != tech_data->display_prerequisite.end(); ++it){
		const auto max_level = castle->get_max_level(it->first);
		if(max_level < it->second){
			LOG_EMPERY_CENTER_DEBUG("Display prerequisite not met: tech_id = ", it->first,
				", level_required = ", it->second, ", max_level = ", max_level);
			return Response(Msg::ERR_DISPLAY_PREREQUISITE_NOT_MET) <<it->first;
		}
	}

	boost::container::flat_map<ItemId, std::uint64_t> tokens;
	tokens.reserve(req.tokens.size());
	for(auto it = req.tokens.begin(); it != req.tokens.end(); ++it){
		auto &count_total = tokens[ItemId(it->item_id)];
		count_total = checked_add<std::uint64_t>(count_total, it->count);
	}

	const auto result = try_decrement_resources(castle, item_box, task_box, tech_data->upgrade_cost, tokens,
		ReasonIds::ID_UPGRADE_TECH, tech_data->tech_id_level.first.get(), tech_data->tech_id_level.second, 0,
		[&]{ castle->create_tech_mission(tech_id, Castle::MIS_UPGRADE); });
	if(result.first){
		return Response(Msg::ERR_CASTLE_NO_ENOUGH_RESOURCES) <<result.first;
	}
	if(result.second){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<result.second;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleCancelTechMission, account, session, req){
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

	const auto info = castle->get_tech(tech_id);
	if(info.mission == Castle::MIS_NONE){
		return Response(Msg::ERR_NO_TECH_MISSION) <<tech_id;
	}

	const auto tech_data = Data::CastleTech::require(info.tech_id, info.tech_level + 1);
	std::vector<ResourceTransactionElement> transaction;
	if((info.mission == Castle::MIS_CONSTRUCT) || (info.mission == Castle::MIS_UPGRADE)){
		const auto refund_ratio = Data::Global::as_double(Data::Global::SLOT_BATTALION_PRODUCTION_CANCELLATION_REFUND_RATIO);
		for(auto it = tech_data->upgrade_cost.begin(); it != tech_data->upgrade_cost.end(); ++it){
			transaction.emplace_back(ResourceTransactionElement::OP_ADD,
				it->first, static_cast<std::uint64_t>(std::floor(it->second * refund_ratio + 0.001)),
				ReasonIds::ID_CANCEL_UPGRADE_TECH, info.tech_id.get(), info.tech_level, 0);
		}
	}
	castle->commit_resource_transaction(transaction,
		[&]{ castle->cancel_tech_mission(tech_id); });

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
		synchronize_map_object_with_player(map_object, session);
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
	if(map_cells.empty()){
		return Response(Msg::ERR_CASTLE_HAS_NO_MAP_CELL) <<map_object_uuid;
	}

	bool warehouse_full = true;
	for(auto it = map_cells.begin(); it != map_cells.end(); ++it){
		const auto &map_cell = *it;
		map_cell->pump_status();

		if(map_cell->get_resource_amount() != 0){
			const auto resource_id = map_cell->get_production_resource_id();
			const auto amount_harvested = map_cell->harvest(false);
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
				account->get_account_uuid(), map_object_uuid, std::string(), coord, utc_now, false);
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

	const auto map_object_uuid_head = Poseidon::load_be(reinterpret_cast<const std::uint64_t &>(map_object_uuid.get()[0]));

	const auto count_to_consume = req.repeat_count;
	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(ItemTransactionElement::OP_REMOVE, item_id, count_to_consume,
		ReasonIds::ID_UNPACK_INTO_CASTLE, map_object_uuid_head, item_id.get(), count_to_consume);
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction, false,
		[&]{
			std::vector<ResourceTransactionElement> res_transaction;
			res_transaction.emplace_back(ResourceTransactionElement::OP_ADD, resource_id, checked_mul(count_to_consume, item_data->value),
				ReasonIds::ID_UNPACK_INTO_CASTLE, map_object_uuid_head, item_id.get(), count_to_consume);
			castle->commit_resource_transaction(res_transaction);
		});
	if(insuff_item_id){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<insuff_item_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleBeginBattalionProduction, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	const auto item_box = ItemBoxMap::require(account->get_account_uuid());
	const auto task_box = TaskBoxMap::require(account->get_account_uuid());

	const auto building_base_id = BuildingBaseId(req.building_base_id);

	const auto building_info = castle->get_building_base(building_base_id);
	if(!building_info.building_id){
		return Response(Msg::ERR_NO_BUILDING_THERE) <<building_base_id;
	}
	if(building_info.mission != Castle::MIS_NONE){
		return Response(Msg::ERR_BARRACKS_UPGRADE_IN_PROGRESS) <<building_base_id;
	}

	const auto info = castle->get_battalion_production(building_base_id);
	if(info.map_object_type_id){
		return Response(Msg::ERR_BATTALION_PRODUCTION_CONFLICT) <<info.map_object_type_id;
	}

	const auto map_object_type_id = MapObjectTypeId(req.map_object_type_id);
	const auto map_object_type_data = Data::MapObjectTypeBattalion::require(map_object_type_id);
	if(building_info.building_id != map_object_type_data->factory_id){
		return Response(Msg::ERR_FACTORY_ID_MISMATCH) <<map_object_type_data->factory_id;
	}
	const auto battalion_info = castle->get_battalion(map_object_type_id);
	if(!(battalion_info.enabled || map_object_type_data->enability_cost.empty())){
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

	boost::container::flat_map<ItemId, std::uint64_t> tokens;
	tokens.reserve(req.tokens.size());
	for(auto it = req.tokens.begin(); it != req.tokens.end(); ++it){
		auto &count_total = tokens[ItemId(it->item_id)];
		count_total = checked_add<std::uint64_t>(count_total, it->count);
	}

	const auto result = try_decrement_resources(castle, item_box, task_box, production_cost, tokens,
		ReasonIds::ID_PRODUCE_BATTALION, map_object_type_id.get(), count, 0,
		[&]{ castle->begin_battalion_production(building_base_id, map_object_type_id, count); });
	if(result.first){
		return Response(Msg::ERR_CASTLE_NO_ENOUGH_RESOURCES) <<result.first;
	}
	if(result.second){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<result.second;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleCancelBattalionProduction, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	const auto building_base_id = BuildingBaseId(req.building_base_id);

	const auto info = castle->get_battalion_production(building_base_id);
	if(!info.map_object_type_id){
		return Response(Msg::ERR_NO_BATTALION_PRODUCTION) <<building_base_id;
	}

	const auto map_object_type_id = info.map_object_type_id;
	const auto map_object_type_data = Data::MapObjectTypeBattalion::require(map_object_type_id);
	const auto count = info.count;

	const auto refund_ratio = Data::Global::as_double(Data::Global::SLOT_CASTLE_UPGRADE_CANCELLATION_REFUND_RATIO);

	std::vector<ResourceTransactionElement> transaction;
	for(auto it = map_object_type_data->production_cost.begin(); it != map_object_type_data->production_cost.end(); ++it){
		transaction.emplace_back(ResourceTransactionElement::OP_ADD,
			it->first, checked_mul(static_cast<std::uint64_t>(std::floor(it->second * refund_ratio + 0.001)), count),
			ReasonIds::ID_CANCEL_PRODUCE_BATTALION, map_object_type_id.get(), count, 0);
	}
	castle->commit_resource_transaction(transaction,
		[&]{ castle->cancel_battalion_production(building_base_id); });

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleHarvestBattalion, account, session, req){
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

	const auto info = castle->get_battalion_production(building_base_id);
	if(!info.map_object_type_id){
		return Response(Msg::ERR_NO_BATTALION_PRODUCTION) <<building_base_id;
	}
	const auto utc_now = Poseidon::get_utc_time();
	if(utc_now < info.production_time_end){
		return Response(Msg::ERR_BATTALION_PRODUCTION_INCOMPLETE) <<building_base_id;
	}

	const auto count_harvested = castle->harvest_battalion(building_base_id);

	try {
		task_box->check(TaskTypeIds::ID_HARVEST_SOLDIERS, info.map_object_type_id.get(), count_harvested,
			castle, 0, 0);
	} catch(std::exception &e){
		LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleSpeedUpBattalionProduction, account, session, req){
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

	const auto info = castle->get_battalion_production(building_base_id);
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
		[&]{ castle->speed_up_battalion_production(building_base_id, saturated_mul(turbo_milliseconds, count_to_consume)); });
	if(insuff_item_id){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<insuff_item_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleEnableBattalion, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	const auto item_box = ItemBoxMap::require(account->get_account_uuid());
	const auto task_box = TaskBoxMap::require(account->get_account_uuid());

	const auto map_object_type_id = MapObjectTypeId(req.map_object_type_id);
	const auto map_object_type_data = Data::MapObjectTypeBattalion::get(map_object_type_id);
	if(!map_object_type_data){
		return Response(Msg::ERR_NO_SUCH_MAP_OBJECT_TYPE) <<map_object_type_id;
	}
	const auto info = castle->get_battalion(map_object_type_id);
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
		const auto prev_info = castle->get_battalion(previous_id);
		if(!(prev_info.enabled || prev_data->enability_cost.empty())){
			return Response(Msg::ERR_PREREQUISITE_BATTALION_NOT_MET) <<previous_id;
		}
	}

	boost::container::flat_map<ItemId, std::uint64_t> tokens;
	tokens.reserve(req.tokens.size());
	for(auto it = req.tokens.begin(); it != req.tokens.end(); ++it){
		auto &count_total = tokens[ItemId(it->item_id)];
		count_total = checked_add<std::uint64_t>(count_total, it->count);
	}

	const auto result = try_decrement_resources(castle, item_box, task_box, map_object_type_data->enability_cost, tokens,
		ReasonIds::ID_ENABLE_BATTALION, map_object_type_id.get(), 0, 0,
		[&]{ castle->enable_battalion(map_object_type_id); });
	if(result.first){
		return Response(Msg::ERR_CASTLE_NO_ENOUGH_RESOURCES) <<result.first;
	}
	if(result.second){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<result.second;
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
	const auto count = req.count;

	const auto map_object_type_data = Data::MapObjectTypeBattalion::get(map_object_type_id);
	if(!map_object_type_data){
		return Response(Msg::ERR_NO_SUCH_MAP_OBJECT_TYPE) <<map_object_type_id;
	}
	const auto max_soldier_count = map_object_type_data->max_soldier_count;
	if(count > max_soldier_count){
		return Response(Msg::ERR_TOO_MANY_SOLDIERS_FOR_BATTALION) <<max_soldier_count;
	}

	std::size_t battalion_count = 0;
	std::vector<boost::shared_ptr<MapObject>> current_battalions;
	WorldMap::get_map_objects_by_parent_object(current_battalions, map_object_uuid);
	for(auto it = current_battalions.begin(); it != current_battalions.end(); ++it){
		const auto &battalion = *it;
		if(battalion->get_map_object_type_id() == MapObjectTypeIds::ID_CASTLE){
			continue;
		}
		++battalion_count;
	}
	const auto max_battalion_count = castle->get_max_battalion_count();
	if(battalion_count >= max_battalion_count){
		return Response(Msg::ERR_MAX_BATTALION_COUNT_EXCEEDED) <<max_battalion_count;
	}

	const auto battalion_uuid = MapObjectUuid(Poseidon::Uuid::random());
	const auto utc_now        = Poseidon::get_utc_time();

	const auto castle_uuid_head    = Poseidon::load_be(reinterpret_cast<const std::uint64_t &>(map_object_uuid.get()[0]));
	const auto battalion_uuid_head = Poseidon::load_be(reinterpret_cast<const std::uint64_t &>(battalion_uuid.get()[0]));

	boost::container::flat_map<AttributeId, std::int64_t> modifiers;
	modifiers[AttributeIds::ID_SOLDIER_COUNT]     = static_cast<std::int64_t>(count);
	modifiers[AttributeIds::ID_SOLDIER_COUNT_MAX] = static_cast<std::int64_t>(count);

	std::vector<SoldierTransactionElement> transaction;
	transaction.emplace_back(SoldierTransactionElement::OP_REMOVE, map_object_type_id, count,
		ReasonIds::ID_CREATE_BATTALION, castle_uuid_head, battalion_uuid_head, 0);
	const auto insuff_soldier_id = castle->commit_soldier_transaction_nothrow(transaction,
		[&]{
			const auto battalion = boost::make_shared<MapObject>(battalion_uuid, map_object_type_id,
				account->get_account_uuid(), map_object_uuid, std::string(), castle->get_coord(), utc_now, true);
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

PLAYER_SERVLET(Msg::CS_CastleCompleteBattalionProductionImmediately, account, session, req){
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

	const auto info = castle->get_battalion_production(building_base_id);
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
		[&]{ castle->speed_up_battalion_production(building_base_id, UINT64_MAX); });
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

	auto deploy_result = can_deploy_castle_at(coord, { });
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
				account->get_account_uuid(), castle->get_map_object_uuid(), std::move(req.name), coord, utc_now);
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

}
