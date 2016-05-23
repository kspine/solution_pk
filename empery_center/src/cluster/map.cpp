#include "../precompiled.hpp"
#include "common.hpp"
#include "../mmain.hpp"
#include "../msg/ks_map.hpp"
#include "../msg/sk_map.hpp"
#include "../msg/sc_map.hpp"
#include "../msg/sc_battle_record.hpp"
#include "../msg/err_map.hpp"
#include "../msg/err_castle.hpp"
#include "../msg/err_account.hpp"
#include "../msg/kill.hpp"
#include <poseidon/json.hpp>
#include <poseidon/async_job.hpp>
#include "../singletons/world_map.hpp"
#include "../map_object.hpp"
#include "../map_object_type_ids.hpp"
#include "../map_utilities.hpp"
#include "../map_utilities_center.hpp"
#include "../data/map.hpp"
#include "../castle.hpp"
#include "../overlay.hpp"
#include "../strategic_resource.hpp"
#include "../data/map_object_type.hpp"
#include "../data/global.hpp"
#include "../attribute_ids.hpp"
#include "../announcement.hpp"
#include "../singletons/announcement_map.hpp"
#include "../chat_message_type_ids.hpp"
#include "../chat_message_slot_ids.hpp"
#include "../singletons/player_session_map.hpp"
#include "../player_session.hpp"
#include "../singletons/account_map.hpp"
#include "../singletons/battle_record_box_map.hpp"
#include "../battle_record_box.hpp"
#include "../singletons/task_box_map.hpp"
#include "../task_box.hpp"
#include "../task_type_ids.hpp"
#include "../singletons/war_status_map.hpp"
#include "../data/castle.hpp"
#include "../transaction_element.hpp"
#include "../reason_ids.hpp"
#include "../singletons/item_box_map.hpp"
#include "../item_box.hpp"
#include "../resource_ids.hpp"
#include "../resource_crate.hpp"
#include "../buff_ids.hpp"
#include "../map_cell.hpp"

namespace EmperyCenter {

CLUSTER_SERVLET(Msg::KS_MapRegisterCluster, cluster, req){
	const auto center_rectangle = WorldMap::get_cluster_scope(Coord(0, 0));
	const auto map_width  = static_cast<std::uint32_t>(center_rectangle.width());
	const auto map_height = static_cast<std::uint32_t>(center_rectangle.height());

	const auto num_coord = Coord(req.numerical_x, req.numerical_y);
	const auto inf_x = INT32_MAX / static_cast<std::int32_t>(map_width);
	const auto inf_y = INT32_MAX / static_cast<std::int32_t>(map_height);
	if((num_coord.x() <= -inf_x) || (inf_x <= num_coord.x()) || (num_coord.y() <= -inf_y) || (inf_y <= num_coord.y())){
		LOG_EMPERY_CENTER_WARNING("Invalid numerical coord: num_coord = ", num_coord, ", inf_x = ", inf_x, ", inf_y = ", inf_y);
		return Response(Msg::KILL_INVALID_NUMERICAL_COORD) <<num_coord;
	}
	const auto cluster_scope = WorldMap::get_cluster_scope(Coord(num_coord.x() * map_width, num_coord.y() * map_height));
	const auto cluster_coord = cluster_scope.bottom_left();
	LOG_EMPERY_CENTER_DEBUG("Registering cluster server: num_coord = ", num_coord, ", cluster_scope = ", cluster_scope);

	WorldMap::set_cluster(cluster, cluster_coord);
	cluster->set_name(std::move(req.name));

	Msg::SK_MapClusterRegistrationSucceeded msg;
	msg.cluster_x = cluster_coord.x();
	msg.cluster_y = cluster_coord.y();
	cluster->send(msg);

	WorldMap::synchronize_cluster(cluster, cluster_scope);

	return Response();
}

CLUSTER_SERVLET(Msg::KS_MapUpdateMapObjectAction, cluster, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto map_object = WorldMap::get_map_object(map_object_uuid);
	if(!map_object){
		return Response(Msg::ERR_NO_SUCH_MAP_OBJECT) <<map_object_uuid;
	}
	const auto test_cluster = WorldMap::get_cluster(map_object->get_coord());
	if(cluster != test_cluster){
		return Response(Msg::ERR_MAP_OBJECT_ON_ANOTHER_CLUSTER);
	}

	const auto old_coord = map_object->get_coord();
	const auto new_coord = Coord(req.x, req.y);
	map_object->set_coord(new_coord); // noexcept

	map_object->set_action(req.action, std::move(req.param));

	const auto new_cluster = WorldMap::get_cluster(new_coord);
	if(!new_cluster){
		LOG_EMPERY_CENTER_DEBUG("No cluster there: new_coord = ", new_coord);
		// 注意，这个不能和上面那个 set_coord() 合并成一个操作。
		// 如果我们跨服务器设定了坐标，在这里地图服务器会重新同步数据，并删除旧的路径。
		map_object->set_coord(old_coord); // noexcept
	}

	return Response();
}
/*
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
*/
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
	const auto resource_data = Data::CastleResource::require(resource_id);
	const auto carried_attribute_id = resource_data->carried_attribute_id;
	if(!carried_attribute_id){
		return Response(Msg::ERR_RESOURCE_NOT_HARVESTABLE) <<resource_id;
	}

	const auto map_object_type_id = map_object->get_map_object_type_id();
	const auto map_object_type_data = Data::MapObjectTypeBattalion::require(map_object_type_id);
	const auto harvest_speed = map_object_type_data->harvest_speed;
	if(harvest_speed <= 0){
		return Response(Msg::ERR_ZERO_HARVEST_SPEED) <<map_object_type_id;
	}
	const auto soldier_count = static_cast<std::uint64_t>(map_object->get_attribute(AttributeIds::ID_SOLDIER_COUNT));
	const auto resource_capacity = static_cast<std::uint64_t>(map_object_type_data->resource_carriable * soldier_count);
	const auto resource_carried = map_object->get_resource_amount_carried();
	if(resource_carried >= resource_capacity){
		return Response(Msg::ERR_CARRIABLE_RESOURCE_LIMIT_EXCEEDED) <<resource_capacity;
	}

	const auto harvest_speed_turbo = castle->get_attribute(AttributeIds::ID_HARVEST_SPEED_BONUS) / 1000.0;

	const auto amount_to_harvest = harvest_speed * (1 + harvest_speed_turbo) * soldier_count * req.interval / 60000.0;
	const auto amount_harvested = overlay->harvest(map_object, amount_to_harvest, false);
	LOG_EMPERY_CENTER_DEBUG("Harvest: map_object_uuid = ", map_object_uuid, ", map_object_type_id = ", map_object_type_id,
		", harvest_speed = ", harvest_speed, ", interval = ", req.interval, ", amount_harvested = ", amount_harvested);

	return Response();
}

CLUSTER_SERVLET(Msg::KS_MapDeployImmigrants, cluster, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto map_object = WorldMap::get_map_object(map_object_uuid);
	if(!map_object){
		return Response(Msg::ERR_NO_SUCH_MAP_OBJECT) <<map_object_uuid;
	}
	const auto coord = map_object->get_coord();
	const auto test_cluster = WorldMap::get_cluster(coord);
	if(cluster != test_cluster){
		return Response(Msg::ERR_MAP_OBJECT_ON_ANOTHER_CLUSTER);
	}

	const auto map_object_type_id = map_object->get_map_object_type_id();
	if(map_object_type_id != MapObjectTypeIds::ID_IMMIGRANTS){
		return Response(Msg::ERR_MAP_OBJECT_IS_NOT_IMMIGRANTS) <<map_object_type_id;
	}

	auto result = can_deploy_castle_at(coord, map_object_uuid);
	if(result.first != 0){
		return std::move(result);
	}

	const auto utc_now = Poseidon::get_utc_time();

	const auto castle_uuid = MapObjectUuid(Poseidon::Uuid::random());
	const auto owner_uuid = map_object->get_owner_uuid();
	const auto castle = boost::make_shared<Castle>(castle_uuid,
		owner_uuid, map_object->get_parent_object_uuid(), std::move(req.castle_name), coord, utc_now);
	castle->check_init_buildings();
	castle->check_init_resources();
	castle->pump_status();
	WorldMap::insert_map_object(castle);
	LOG_EMPERY_CENTER_INFO("Created castle: castle_uuid = ", castle_uuid, ", owner_uuid = ", owner_uuid);
	map_object->delete_from_game(); // noexcept

	try {
		const auto announcement_uuid = AnnouncementUuid(Poseidon::Uuid::random());
		const auto language_id       = LanguageId();

		std::vector<std::pair<ChatMessageSlotId, std::string>> segments;
		segments.reserve(4);
		segments.emplace_back(ChatMessageSlotIds::ID_NEW_CASTLE_OWNER,   owner_uuid.str());
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

CLUSTER_SERVLET(Msg::KS_MapEnterCastle, cluster, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto map_object = WorldMap::get_map_object(map_object_uuid);
	if(!map_object){
		return Response(Msg::ERR_NO_SUCH_MAP_OBJECT) <<map_object_uuid;
	}
	const auto test_cluster = WorldMap::get_cluster(map_object->get_coord());
	if(cluster != test_cluster){
		return Response(Msg::ERR_MAP_OBJECT_ON_ANOTHER_CLUSTER);
	}

	const auto castle_uuid = map_object->get_parent_object_uuid();
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(castle_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<castle_uuid;
	}
	if(castle->get_owner_uuid() != map_object->get_owner_uuid()){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	bool close_enough = false;
	std::vector<Coord> foundation;
	get_castle_foundation(foundation, castle->get_coord(), true);
	for(auto it = foundation.begin(); it != foundation.end(); ++it){
		if(map_object->get_coord() == *it){
			close_enough = true;
			break;
		}
	}
	if(!close_enough){
		return Response(Msg::ERR_TOO_FAR_FROM_CASTLE);
	}

	map_object->unload_resources(castle);

	map_object->set_coord(castle->get_coord());
	map_object->set_garrisoned(true);

	return Response();
}

namespace {
	template<typename T, typename PredT>
	void update_attributes_single(const boost::shared_ptr<T> &ptr, const PredT &pred){
		if(pred()){
			const auto parent_object_uuid = ptr->get_parent_object_uuid();
			if(parent_object_uuid){
				const auto parent_object = WorldMap::get_map_object(parent_object_uuid);
				if(parent_object){
					parent_object->recalculate_attributes(false);
				}
			}
		}
		ptr->recalculate_attributes(false);
	}
}

CLUSTER_SERVLET(Msg::KS_MapHarvestStrategicResource, cluster, req){
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

	const auto strategic_resource = WorldMap::get_strategic_resource(coord);
	if(!strategic_resource){
		return Response(Msg::ERR_STRATEGIC_RESOURCE_ALREADY_REMOVED) <<coord;
	}
	const auto resource_amount = strategic_resource->get_resource_amount();
	if(resource_amount == 0){
		return Response(Msg::ERR_STRATEGIC_RESOURCE_ALREADY_REMOVED) <<coord;
	}
	const auto resource_id = strategic_resource->get_resource_id();
	const auto resource_data = Data::CastleResource::require(resource_id);
	const auto carried_attribute_id = resource_data->carried_attribute_id;
	if(!carried_attribute_id){
		return Response(Msg::ERR_RESOURCE_NOT_HARVESTABLE) <<resource_id;
	}

	const auto map_object_type_id = map_object->get_map_object_type_id();
	const auto map_object_type_data = Data::MapObjectTypeBattalion::require(map_object_type_id);
	const auto harvest_speed = map_object_type_data->harvest_speed;
	if(harvest_speed <= 0){
		return Response(Msg::ERR_ZERO_HARVEST_SPEED) <<map_object_type_id;
	}
	const auto soldier_count = static_cast<std::uint64_t>(map_object->get_attribute(AttributeIds::ID_SOLDIER_COUNT));
	const auto resource_capacity = static_cast<std::uint64_t>(map_object_type_data->resource_carriable * soldier_count);
	const auto resource_carried = map_object->get_resource_amount_carried();
	if(resource_carried >= resource_capacity){
		return Response(Msg::ERR_CARRIABLE_RESOURCE_LIMIT_EXCEEDED) <<resource_capacity;
	}

	const auto harvest_speed_turbo = castle->get_attribute(AttributeIds::ID_HARVEST_SPEED_BONUS) / 1000.0;

	const auto amount_to_harvest = harvest_speed * (1 + harvest_speed_turbo) * soldier_count * req.interval / 60000.0;
	const auto amount_harvested = strategic_resource->harvest(map_object, amount_to_harvest, false);
	LOG_EMPERY_CENTER_DEBUG("Harvest: map_object_uuid = ", map_object_uuid, ", map_object_type_id = ", map_object_type_id,
		", harvest_speed = ", harvest_speed, ", interval = ", req.interval, ", amount_harvested = ", amount_harvested);

	return Response();
}

namespace {
	template<typename T>
	bool is_protection_in_effect(const boost::shared_ptr<T> &ptr){
		PROFILE_ME;

		if(ptr->is_buff_in_effect(BuffIds::ID_CASTLE_PROTECTION) && !ptr->is_buff_in_effect(BuffIds::ID_CASTLE_PROTECTION_PREPARATION)){
			return true;
		}
		if(ptr->is_buff_in_effect(BuffIds::ID_OCCUPATION_PROTECTION)){
			return true;
		}
		return false;
	}

	bool is_map_object_protection_in_effect(const boost::shared_ptr<MapObject> &map_object){
		PROFILE_ME;

		const auto defense = boost::dynamic_pointer_cast<DefenseBuilding>(map_object);
		if(!defense){
			return false;
		}
		return is_protection_in_effect(defense);
	}
	bool is_map_cell_protection_in_effect(const boost::shared_ptr<MapCell> &map_cell){
		PROFILE_ME;

		const auto ticket_item_id = map_cell->get_ticket_item_id();
		const auto ticket_data = Data::MapCellTicket::require(ticket_item_id);
		if(!ticket_data->protectable){
			return false;
		}
		return is_protection_in_effect(map_cell);
	}

	enum BattleNotificationType : int {
		NOTIFY_ATTACK_MAP_OBJECT  = 1,
		NOTIFY_KILL_MAP_OBJECT    = 2,
		NOTIFY_ATTACK_MAP_CELL    = 3,
		NOTIFY_CAPTURE_MAP_CELL   = 4,
		NOTIFY_ATTACK_CASTLE      = 5,
		NOTIFY_CAPTURE_CASTLE     = 6,
	};

	template<typename ...ParamsT>
	void send_battle_notification(AccountUuid account_uuid, int type, AccountUuid other_account_uuid, Coord coord,
		const ParamsT &...params)
	{
		PROFILE_ME;

		const auto session = PlayerSessionMap::get(account_uuid);
		if(!session){
			return;
		}

		try {
			const auto utc_now = Poseidon::get_utc_time();

			if(other_account_uuid){
				AccountMap::cached_synchronize_account_with_player(other_account_uuid, session);
			}

			Msg::SC_BattleRecordNotification msg;
			msg.type               = static_cast<int>(type);
			msg.timestamp          = utc_now;
			msg.other_account_uuid = other_account_uuid.str();
			msg.coord_x            = coord.x();
			msg.coord_y            = coord.y();
			msg.params.reserve(sizeof...(params));
			std::string param_strs[] = { boost::lexical_cast<std::string>(params)... };
			for(std::size_t i = 0; i < sizeof...(params); ++i){
				auto &param = *msg.params.emplace(msg.params.end());
				param.str = std::move(param_strs[i]);
			}
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
}

CLUSTER_SERVLET(Msg::KS_MapObjectAttackAction, cluster, req){
	const auto attacking_object_uuid = MapObjectUuid(req.attacking_object_uuid);
	const auto attacked_object_uuid  = MapObjectUuid(req.attacked_object_uuid);

	// 结算战斗伤害。
	const auto attacking_object = WorldMap::get_map_object(attacking_object_uuid);
	if(!attacking_object){
		return Response(Msg::ERR_NO_SUCH_MAP_OBJECT) <<attacking_object_uuid;
	}
	const auto attacked_object = WorldMap::get_map_object(attacked_object_uuid);
	if(!attacked_object){
		return Response(Msg::ERR_NO_SUCH_MAP_OBJECT) <<attacked_object_uuid;
	}

	if(is_map_object_protection_in_effect(attacking_object)){
		return Response(Msg::ERR_BATTALION_UNDER_PROTECTION) <<attacking_object_uuid;
	}
	if(is_map_object_protection_in_effect(attacked_object)){
		return Response(Msg::ERR_BATTALION_UNDER_PROTECTION) <<attacked_object_uuid;
	}

	const auto utc_now = Poseidon::get_utc_time();

	const auto attacking_object_type_id = attacking_object->get_map_object_type_id();
	const auto attacking_account_uuid = attacking_object->get_owner_uuid();
	const auto attacking_coord = attacking_object->get_coord();

	const auto attacked_object_type_id = attacked_object->get_map_object_type_id();
	const auto attacked_account_uuid = attacked_object->get_owner_uuid();
	const auto attacked_coord = attacked_object->get_coord();

	if(attacking_account_uuid == attacked_account_uuid){
		return Response(Msg::ERR_CANNOT_ATTACK_FRIENDLY_OBJECTS);
	}

	update_attributes_single(attacking_object, [&]{ return attacking_object_type_id != MapObjectTypeIds::ID_CASTLE; });
	update_attributes_single(attacked_object, [&]{ return attacked_object_type_id != MapObjectTypeIds::ID_CASTLE; });

	const auto result_type = req.result_type;
	const auto soldiers_previous = static_cast<std::uint64_t>(attacked_object->get_attribute(AttributeIds::ID_SOLDIER_COUNT));
	const auto soldiers_damaged = std::min(soldiers_previous, req.soldiers_damaged);
	const auto soldiers_remaining = checked_sub(soldiers_previous, soldiers_damaged);
	LOG_EMPERY_CENTER_DEBUG("Map object damaged: attacked_object_uuid = ", attacked_object_uuid,
		", soldiers_previous = ", soldiers_previous, ", soldiers_damaged = ", soldiers_damaged, ", soldiers_remaining = ", soldiers_remaining);

	boost::container::flat_map<AttributeId, std::int64_t> modifiers;
	modifiers[AttributeIds::ID_SOLDIER_COUNT] = static_cast<std::int64_t>(soldiers_remaining);
	attacked_object->set_attributes(std::move(modifiers));

	if(soldiers_remaining <= 0){
		LOG_EMPERY_CENTER_DEBUG("Map object is dead now: attacked_object_uuid = ", attacked_object_uuid);
		if(attacked_object_type_id == MapObjectTypeIds::ID_CASTLE){
			const auto protection_minutes = Data::Global::as_unsigned(Data::Global::SLOT_CASTLE_SIEGE_PROTECTION_DURATION);
			const auto protection_duration = saturated_mul<std::uint64_t>(protection_minutes, 60000);
			attacked_object->set_buff(BuffIds::ID_OCCUPATION_PROTECTION, utc_now, protection_duration);
		} else {
			attacked_object->delete_from_game();
		}
	}

	// 回收伤兵。
	std::uint64_t soldiers_wounded = 0, soldiers_wounded_added = 0;
	{
		PROFILE_ME;

		const auto battalion_type_data = Data::MapObjectTypeBattalion::get(attacked_object_type_id);
		if(!battalion_type_data){
			goto _wounded_done;
		}
		if(battalion_type_data->speed <= 0){
			goto _wounded_done;
		}

		const auto parent_object_uuid = attacked_object->get_parent_object_uuid();
		if(!parent_object_uuid){
			goto _wounded_done;
		}
		const auto parent_castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(parent_object_uuid));
		if(!parent_castle){
			LOG_EMPERY_CENTER_WARNING("No such castle: parent_object_uuid = ", parent_object_uuid);
			goto _wounded_done;
		}

		const auto capacity_total = parent_castle->get_medical_tent_capacity();
		if(capacity_total == 0){
			LOG_EMPERY_CENTER_DEBUG("No medical tent: parent_object_uuid = ", parent_object_uuid);
			goto _wounded_done;
		}

		const auto wounded_ratio_basic = Data::Global::as_double(Data::Global::SLOT_WOUNDED_SOLDIER_RATIO_BASIC_VALUE);
		const auto wounded_ratio_bonus = parent_castle->get_attribute(AttributeIds::ID_WOUNDED_SOLDIER_RATIO_BONUS);
		auto wounded_ratio = wounded_ratio_basic + wounded_ratio_bonus / 1000.0;
		if(wounded_ratio < 0){
			wounded_ratio = 0;
		} else if(wounded_ratio > 1){
			wounded_ratio = 1;
		}
		soldiers_wounded = static_cast<std::uint64_t>(soldiers_damaged * wounded_ratio + 0.001);
		LOG_EMPERY_CENTER_DEBUG("Wounded soldiers: wounded_ratio_basic = ", wounded_ratio_basic, ", wounded_ratio_bonus = ", wounded_ratio_bonus,
			", soldiers_damaged = ", soldiers_damaged, ", soldiers_wounded = ", soldiers_wounded);

		std::uint64_t capacity_used = 0;
		std::vector<Castle::WoundedSoldierInfo> wounded_soldier_all;
		parent_castle->get_all_wounded_soldiers(wounded_soldier_all);
		for(auto it = wounded_soldier_all.begin(); it != wounded_soldier_all.end(); ++it){
			capacity_used = checked_add(capacity_used, it->count);
		}
		const auto capacity_avail = saturated_sub(capacity_total, capacity_used);
		if(capacity_avail == 0){
			LOG_EMPERY_CENTER_DEBUG("Medical tent is full: parent_object_uuid = ", parent_object_uuid,
				", capacity_total = ", capacity_total, ", capacity_used = ", capacity_used, ", capacity_avail = ", capacity_avail);
			goto _wounded_done;
		}
		soldiers_wounded_added = std::min(soldiers_wounded, capacity_avail);

		const auto attacked_object_uuid_head = Poseidon::load_be(reinterpret_cast<const std::uint64_t &>(attacked_object_uuid.get()[0]));

		std::vector<WoundedSoldierTransactionElement> wounded_soldier_transaction;
		wounded_soldier_transaction.emplace_back(WoundedSoldierTransactionElement::OP_ADD, attacked_object_type_id, soldiers_wounded_added,
			ReasonIds::ID_SOLDIER_WOUNDED, attacked_object_uuid_head, soldiers_damaged, wounded_ratio_bonus);
		parent_castle->commit_wounded_soldier_transaction(wounded_soldier_transaction);
	}
_wounded_done:
	;

	const auto battle_status_timeout = get_config<std::uint64_t>("battle_status_timeout", 10000);
	attacking_object->set_buff(BuffIds::ID_BATTLE_STATUS, utc_now, battle_status_timeout);
	attacked_object->set_buff(BuffIds::ID_BATTLE_STATUS, utc_now, battle_status_timeout);

	const auto should_send_battle_notifications = !attacked_object->is_buff_in_effect(BuffIds::ID_BATTLE_NOTIFICATION_TIMEOUT);
	attacked_object->set_buff(BuffIds::ID_BATTLE_NOTIFICATION_TIMEOUT, utc_now, battle_status_timeout);

	const auto category = boost::make_shared<int>();

	// 通知客户端。
	try {
		PROFILE_ME;

		Msg::SC_MapObjectAttackResult msg;
		msg.attacking_object_uuid  = attacking_object_uuid.str();
		msg.attacking_coord_x      = attacking_coord.x();
		msg.attacking_coord_y      = attacking_coord.y();
		msg.attacked_object_uuid   = attacked_object_uuid.str();
		msg.attacked_coord_x       = attacked_coord.x();
		msg.attacked_coord_y       = attacked_coord.y();
		msg.result_type            = result_type;
		msg.soldiers_wounded       = soldiers_wounded;
		msg.soldiers_wounded_added = soldiers_wounded_added;
		msg.soldiers_damaged       = soldiers_damaged;
		msg.soldiers_remaining     = soldiers_remaining;
		LOG_EMPERY_CENTER_TRACE("Broadcasting attack result message: msg = ", msg);

		const auto range_left   = std::min(attacking_coord.x(), attacked_coord.x());
		const auto range_right  = std::max(attacking_coord.x(), attacked_coord.x());
		const auto range_bottom = std::min(attacking_coord.y(), attacked_coord.y());
		const auto range_top    = std::max(attacking_coord.y(), attacked_coord.y());
		std::vector<boost::shared_ptr<PlayerSession>> sessions;
		WorldMap::get_players_viewing_rectangle(sessions,
			Rectangle(Coord(range_left, range_bottom), Coord(range_right + 1, range_top + 1)));
		for(auto it = sessions.begin(); it != sessions.end(); ++it){
			const auto &session = *it;
			try {
				session->send(msg);
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			}
		}
	} catch(std::exception &e){
		LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
	}

	// 战斗通知。
	if(should_send_battle_notifications){
		const auto attacked_castle = boost::dynamic_pointer_cast<Castle>(attacked_object);
		if(attacked_castle){
			const int type = (soldiers_remaining > 0) ? NOTIFY_ATTACK_CASTLE : NOTIFY_CAPTURE_CASTLE;

			if(attacking_account_uuid){
				try {
					PROFILE_ME;

					send_battle_notification(attacking_account_uuid, type, attacked_account_uuid, attacking_coord,
						attacking_object_type_id, attacked_castle->get_name(), attacked_castle->get_level());
				} catch(std::exception &e){
					LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
				}
			}
			if(attacked_account_uuid){
				try {
					PROFILE_ME;

					send_battle_notification(attacked_account_uuid, -type, attacking_account_uuid, attacked_coord,
						attacking_object_type_id, attacked_object_type_id);
				} catch(std::exception &e){
					LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
				}
			}
		} else {
			const int type = (soldiers_remaining > 0) ? NOTIFY_ATTACK_MAP_OBJECT : NOTIFY_KILL_MAP_OBJECT;

			if(attacking_account_uuid){
				try {
					PROFILE_ME;

					send_battle_notification(attacking_account_uuid, type, attacked_account_uuid, attacking_coord,
						attacking_object_type_id, attacked_object_type_id);
				} catch(std::exception &e){
					LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
				}
			}
			if(attacked_account_uuid){
				try {
					PROFILE_ME;

					send_battle_notification(attacked_account_uuid, -type, attacking_account_uuid, attacked_coord,
						attacking_object_type_id, attacked_castle->get_name(), attacked_castle->get_level());
				} catch(std::exception &e){
					LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
				}
			}
		}
	}

	// 更新交战状态。
	try {
		PROFILE_ME;

		const auto state_persistence_duration = Data::Global::as_double(Data::Global::SLOT_WAR_STATE_PERSISTENCE_DURATION);

		WarStatusMap::set(attacking_account_uuid, attacked_account_uuid,
			saturated_add(utc_now, static_cast<std::uint64_t>(state_persistence_duration * 60000)));
	} catch(std::exception &e){
		LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
	}

	// 战报。
	if(attacking_account_uuid){
		try {
			Poseidon::enqueue_async_job([=]{
				PROFILE_ME;

				const auto battle_record_box = BattleRecordBoxMap::require(attacking_account_uuid);

				battle_record_box->push(utc_now, attacking_object_type_id, attacking_coord,
					attacked_account_uuid, attacked_object_type_id, attacked_coord,
					result_type, soldiers_wounded, soldiers_wounded_added, soldiers_damaged, soldiers_remaining);
			});
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
		}
	}
	if(attacked_account_uuid){
		try {
			Poseidon::enqueue_async_job([=]{
				PROFILE_ME;

				const auto battle_record_box = BattleRecordBoxMap::require(attacked_account_uuid);

				battle_record_box->push(utc_now, attacked_object_type_id, attacked_coord,
					attacking_account_uuid, attacking_object_type_id, attacking_coord,
					-result_type, soldiers_wounded, soldiers_wounded_added, soldiers_damaged, soldiers_remaining);
			});
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
		}
	}

	// 怪物掉落。
	if(attacking_account_uuid && (soldiers_remaining == 0)){
		try {
			Poseidon::enqueue_async_job([=]{
				PROFILE_ME;

				const auto monster_type_data = Data::MapObjectTypeMonster::get(attacked_object_type_id);
				if(!monster_type_data){
					return;
				}

				const auto item_box = ItemBoxMap::require(attacking_account_uuid);

				const auto parent_object_uuid = attacking_object->get_parent_object_uuid();
				const auto parent_castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(parent_object_uuid));
				if(!parent_castle){
					LOG_EMPERY_CENTER_WARNING("No such castle: parent_object_uuid = ", parent_object_uuid);
					return;
				}

				const auto utc_now = Poseidon::get_utc_time();

				boost::container::flat_map<ItemId, std::uint64_t> items_basic, items_extra;

				const auto reward_counter = parent_castle->get_resource(ResourceIds::ID_MONSTER_REWARD_COUNT).amount;
				if(reward_counter > 0){
					std::vector<ResourceTransactionElement> resource_transaction;
					resource_transaction.emplace_back(ResourceTransactionElement::OP_REMOVE, ResourceIds::ID_MONSTER_REWARD_COUNT, 1,
						ReasonIds::ID_MONSTER_REWARD_COUNT, attacked_object_type_id.get(), 0, 0);

					std::vector<ItemTransactionElement> transaction;

					const auto push_monster_rewards = [&](const boost::container::flat_map<std::string, std::uint64_t> &monster_rewards, bool extra){
						for(auto rit = monster_rewards.begin(); rit != monster_rewards.end(); ++rit){
							const auto &collection_name = rit->first;
							const auto repeat_count = rit->second;
							for(std::size_t i = 0; i < repeat_count; ++i){
								const auto reward_data = Data::MapObjectTypeMonsterReward::random_by_collection_name(collection_name);
								if(!reward_data){
									LOG_EMPERY_CENTER_WARNING("Error getting random reward: attacked_object_type_id = ", attacked_object_type_id,
										", collection_name = ", collection_name);
									continue;
								}
								for(auto it = reward_data->reward_items.begin(); it != reward_data->reward_items.end(); ++it){
									const auto item_id = it->first;
									const auto count = it->second;

									if(!extra){
										transaction.emplace_back(ItemTransactionElement::OP_ADD, item_id, count,
											ReasonIds::ID_MONSTER_REWARD, attacked_object_type_id.get(),
											static_cast<std::int64_t>(reward_data->unique_id), 0);
										items_basic[item_id] += count;
									} else {
										transaction.emplace_back(ItemTransactionElement::OP_ADD, item_id, count,
											ReasonIds::ID_MONSTER_REWARD_EXTRA, attacked_object_type_id.get(),
											static_cast<std::int64_t>(reward_data->unique_id), 0);
										items_extra[item_id] += count;
									}
								}
							}
						}
					};

					push_monster_rewards(monster_type_data->monster_rewards, false);

					std::vector<boost::shared_ptr<const Data::MapObjectTypeMonsterRewardExtra>> extra_rewards;
					Data::MapObjectTypeMonsterRewardExtra::get_available(extra_rewards, utc_now, attacked_object_type_id);
					for(auto it = extra_rewards.begin(); it != extra_rewards.end(); ++it){
						const auto &extra_reward_data = *it;
						push_monster_rewards(extra_reward_data->monster_rewards, true);
					}

					parent_castle->commit_resource_transaction(resource_transaction,
						[&]{ item_box->commit_transaction(transaction, false); });
				}

				const auto session = PlayerSessionMap::get(attacking_account_uuid);
				if(session){
					try {
						Msg::SC_MapMonsterRewardGot msg;
						msg.x                  = attacked_coord.x();
						msg.y                  = attacked_coord.y();
						msg.map_object_type_id = attacked_object_type_id.get();
						msg.items_basic.reserve(items_basic.size());
						for(auto it = items_basic.begin(); it != items_basic.end(); ++it){
							auto &elem = *msg.items_basic.emplace(msg.items_basic.end());
							elem.item_id = it->first.get();
							elem.count   = it->second;
						}
						msg.items_extra.reserve(items_extra.size());
						for(auto it = items_extra.begin(); it != items_extra.end(); ++it){
							auto &elem = *msg.items_extra.emplace(msg.items_extra.end());
							elem.item_id = it->first.get();
							elem.count   = it->second;
						}
						msg.castle_uuid        = parent_castle->get_map_object_uuid().str();
						msg.reward_counter     = reward_counter;
						session->send(msg);
					} catch(std::exception &e){
						LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
						session->shutdown(e.what());
					}
				}
			});
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
		}
	}

	// 任务。
	if(attacking_account_uuid && (soldiers_remaining == 0)){
		try {
			Poseidon::enqueue_async_job([=]{
				PROFILE_ME;

				const auto task_box = TaskBoxMap::require(attacking_account_uuid);

				const auto primary_castle_uuid = WorldMap::get_primary_castle_uuid(attacking_account_uuid);

				auto task_type_id = TaskTypeIds::ID_WIPE_OUT_MONSTERS;
				if(attacked_account_uuid){
					task_type_id = TaskTypeIds::ID_WIPE_OUT_ENEMY_BATTALIONS;
				}
				auto castle_category = TaskBox::TCC_PRIMARY;
				if((attacking_object_uuid != primary_castle_uuid) && (attacking_object->get_parent_object_uuid() != primary_castle_uuid)){
					castle_category = TaskBox::TCC_NON_PRIMARY;
				}
				task_box->check(task_type_id, attacked_object_type_id.get(), 1,
					castle_category, 0, 0);
			});
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
		}
	}

	// 资源宝箱。
	if(soldiers_remaining == 0){
		try {
			Poseidon::enqueue_async_job([=]{
				PROFILE_ME;

				const auto &radius_limits = Data::Global::as_array(Data::Global::SLOT_RESOURCE_CRATE_RADIUS_LIMITS);
				const auto radius_inner = static_cast<unsigned>(radius_limits.at(0).get<double>());
				const auto radius_outer = static_cast<unsigned>(radius_limits.at(1).get<double>());

				boost::container::flat_map<ResourceId, std::uint64_t> resources_dropped;
				resources_dropped.reserve(16);

				{
					const auto castle = boost::dynamic_pointer_cast<Castle>(attacked_object);
					if(castle){
						std::vector<Castle::ResourceInfo> resources;
						castle->get_all_resources(resources);
						for(auto it = resources.begin(); it != resources.end(); ++it){
							const auto resource_id = it->resource_id;
							const auto resource_data = Data::CastleResource::get(resource_id);
							if(!resource_data){
								continue;
							}
							if(!resource_data->carried_attribute_id){
								continue;
							}

							const auto amount_protected = castle->get_warehouse_protection(resource_id);
							const auto amount_dropped = saturated_sub(it->amount, amount_protected);

							auto &amount = resources_dropped[resource_id];
							amount = saturated_add(amount, amount_dropped);
						}
						goto _create_crates;
					}
				}
				{
					const auto defense_building = boost::dynamic_pointer_cast<DefenseBuilding>(attacked_object);
					if(defense_building){
						const auto building_level = defense_building->get_level();
						const auto defense_data = Data::MapDefenseBuildingAbstract::get(attacked_object_type_id, building_level);
						if(!defense_data){
							goto _create_crates;
						}
						for(auto it = defense_data->debris.begin(); it != defense_data->debris.end(); ++it){
							const auto resource_id = it->first;
							const auto amount_dropped = it->second;

							auto &amount = resources_dropped[resource_id];
							amount = saturated_add(amount, amount_dropped);
						}
						goto _create_crates;
					}
				}
				{
					boost::container::flat_map<AttributeId, std::int64_t> attributes;
					attacked_object->get_attributes(attributes);
					for(auto it = attributes.begin(); it != attributes.end(); ++it){
						const auto attribute_id = it->first;
						const auto resource_data = Data::CastleResource::get_by_carried_attribute_id(attribute_id);
						if(!resource_data){
							continue;
						}
						if(!resource_data->carried_attribute_id){
							continue;
						}
						const auto resource_id = resource_data->resource_id;

						const auto value = it->second;
						if(value <= 0){
							continue;
						}
						const auto amount_dropped = static_cast<std::uint64_t>(value);

						auto &amount = resources_dropped[resource_id];
						amount = saturated_add(amount, amount_dropped);
					}
				}
			_create_crates:
				;

				for(auto it = resources_dropped.begin(); it != resources_dropped.end(); ++it){
					const auto resource_id = it->first;
					const auto amount = it->second;
					try {
						create_resource_crates(attacked_object->get_coord(), resource_id, amount, radius_inner, radius_outer);
					} catch(std::exception &e){
						LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
					}
				}
			});
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
		}
	}

	return Response();
}

CLUSTER_SERVLET(Msg::KS_MapHealMonster, cluster, req){
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
	const auto monster_data = Data::MapObjectTypeMonster::require(map_object_type_id);

	auto soldier_count = static_cast<std::int64_t>(monster_data->max_soldier_count);
	if(soldier_count < 1){
		soldier_count = 1;
	}

	boost::container::flat_map<AttributeId, std::int64_t> modifiers;
	modifiers.reserve(8);
	modifiers[AttributeIds::ID_SOLDIER_COUNT]         = soldier_count;
	modifiers[AttributeIds::ID_SOLDIER_COUNT_MAX]     = soldier_count;
	map_object->set_attributes(std::move(modifiers));

	return Response();
}

CLUSTER_SERVLET(Msg::KS_MapHarvestResourceCrate, cluster, req){
	const auto attacking_object_uuid = MapObjectUuid(req.attacking_object_uuid);
	const auto resource_crate_uuid   = ResourceCrateUuid(req.resource_crate_uuid);

	const auto attacking_object = WorldMap::get_map_object(attacking_object_uuid);
	if(!attacking_object){
		return Response(Msg::ERR_NO_SUCH_MAP_OBJECT) <<attacking_object_uuid;
	}
	const auto resource_crate = WorldMap::get_resource_crate(resource_crate_uuid);
	if(!resource_crate){
		return Response(Msg::ERR_RESOURCE_CRATE_NOT_FOUND) <<resource_crate_uuid;
	}

	auto amount_remaining = resource_crate->get_amount_remaining();
	if(amount_remaining == 0){
		return Response(Msg::ERR_RESOURCE_CRATE_EMPTY) <<resource_crate_uuid;
	}
	const auto resource_id = resource_crate->get_resource_id();
	const auto resource_data = Data::CastleResource::require(resource_id);
	const auto carried_attribute_id = resource_data->carried_attribute_id;
	if(!carried_attribute_id){
		return Response(Msg::ERR_RESOURCE_NOT_HARVESTABLE) <<resource_id;
	}

	const auto attacking_object_type_id = attacking_object->get_map_object_type_id();
	const auto attacking_coord = attacking_object->get_coord();

	const auto attacking_object_type_data = Data::MapObjectTypeBattalion::require(attacking_object_type_id);
	const auto soldier_count = static_cast<std::uint64_t>(attacking_object->get_attribute(AttributeIds::ID_SOLDIER_COUNT));
	const auto resource_capacity = static_cast<std::uint64_t>(attacking_object_type_data->resource_carriable * soldier_count);
	const auto resource_carried = attacking_object->get_resource_amount_carried();
	if(resource_carried >= resource_capacity){
		return Response(Msg::ERR_CARRIABLE_RESOURCE_LIMIT_EXCEEDED) <<resource_capacity;
	}

	const auto amount_to_harvest = req.amount_harvested;
	const auto amount_harvested = resource_crate->harvest(attacking_object, amount_to_harvest, false);
	LOG_EMPERY_CENTER_DEBUG("Harvest: attacking_object_uuid = ", attacking_object_uuid, ", attacking_object_type_id = ", attacking_object_type_id,
		", amount_to_harvest = ", amount_to_harvest, ", amount_harvested = ", amount_harvested);
	amount_remaining = resource_crate->get_amount_remaining();

	const auto attacked_coord = resource_crate->get_coord();

	// 通知客户端。
	try {
		PROFILE_ME;

		Msg::SC_MapResourceCrateHarvestResult msg;
		msg.attacking_object_uuid  = attacking_object_uuid.str();
		msg.attacking_coord_x      = attacking_coord.x();
		msg.attacking_coord_y      = attacking_coord.y();
		msg.resource_crate_uuid    = resource_crate_uuid.str();
		msg.attacked_coord_x       = attacked_coord.x();
		msg.attacked_coord_y       = attacked_coord.y();
		msg.resource_id            = resource_id.get();
		msg.amount_harvested       = amount_harvested;
		msg.amount_remaining       = amount_remaining;
		LOG_EMPERY_CENTER_TRACE("Broadcasting harvest result message: msg = ", msg);

		const auto range_left   = std::min(attacking_coord.x(), attacked_coord.x());
		const auto range_right  = std::max(attacking_coord.x(), attacked_coord.x());
		const auto range_bottom = std::min(attacking_coord.y(), attacked_coord.y());
		const auto range_top    = std::max(attacking_coord.y(), attacked_coord.y());
		std::vector<boost::shared_ptr<PlayerSession>> sessions;
		WorldMap::get_players_viewing_rectangle(sessions,
			Rectangle(Coord(range_left, range_bottom), Coord(range_right + 1, range_top + 1)));
		for(auto it = sessions.begin(); it != sessions.end(); ++it){
			const auto &session = *it;
			try {
				session->send(msg);
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			}
		}
	} catch(std::exception &e){
		LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
	}

	return Response();
}

CLUSTER_SERVLET(Msg::KS_MapAttackMapCellAction, cluster, req){
	const auto attacking_object_uuid = MapObjectUuid(req.attacking_object_uuid);
	const auto attacked_coord        = Coord(req.attacked_coord_x, req.attacked_coord_y);

	// 结算战斗伤害。
	const auto attacking_object = WorldMap::get_map_object(attacking_object_uuid);
	if(!attacking_object){
		return Response(Msg::ERR_NO_SUCH_MAP_OBJECT) <<attacking_object_uuid;
	}
	const auto attacked_cell = WorldMap::get_map_cell(attacked_coord);
	if(!attacked_cell){
		return Response(Msg::ERR_NO_TICKET_ON_MAP_CELL) <<attacked_coord;
	}
	if(!attacked_cell->get_owner_uuid()){
		return Response(Msg::ERR_NO_TICKET_ON_MAP_CELL) <<attacked_coord;
	}

	if(is_map_object_protection_in_effect(attacking_object)){
		return Response(Msg::ERR_BATTALION_UNDER_PROTECTION) <<attacking_object_uuid;
	}
	if(is_map_cell_protection_in_effect(attacked_cell)){
		return Response(Msg::ERR_MAP_CELL_UNDER_PROTECTION) <<attacked_coord;
	}

	const auto utc_now = Poseidon::get_utc_time();

	const auto attacking_object_type_id = attacking_object->get_map_object_type_id();
	const auto attacking_account_uuid = attacking_object->get_owner_uuid();
	const auto attacking_coord = attacking_object->get_coord();

	const auto attacked_account_uuid = attacked_cell->get_owner_uuid();

	const auto occupier_owner_uuid = attacked_cell->get_occupier_owner_uuid();
	if(occupier_owner_uuid){
		if(attacking_account_uuid == occupier_owner_uuid){
			return Response(Msg::ERR_CANNOT_ATTACK_FRIENDLY_OBJECTS);
		}
	} else {
		if(attacking_account_uuid == attacked_account_uuid){
			return Response(Msg::ERR_CANNOT_ATTACK_FRIENDLY_OBJECTS);
		}
	}

	update_attributes_single(attacking_object, [&]{ return attacking_object_type_id != MapObjectTypeIds::ID_CASTLE; });
	update_attributes_single(attacked_cell, [&]{ return true; });

	const auto soldiers_previous = static_cast<std::uint64_t>(attacked_cell->get_attribute(AttributeIds::ID_SOLDIER_COUNT));
	const auto soldiers_damaged = std::min(soldiers_previous, req.soldiers_damaged);
	const auto soldiers_remaining = checked_sub(soldiers_previous, soldiers_damaged);
	LOG_EMPERY_CENTER_DEBUG("Map cell damaged: attacked_coord = ", attacked_coord,
		", soldiers_previous = ", soldiers_previous, ", soldiers_damaged = ", soldiers_damaged, ", soldiers_remaining = ", soldiers_remaining);

	boost::container::flat_map<AttributeId, std::int64_t> modifiers;
	modifiers[AttributeIds::ID_SOLDIER_COUNT] = static_cast<std::int64_t>(soldiers_remaining);
	attacked_cell->set_attributes(std::move(modifiers));

	const bool is_occupied = attacked_cell->is_buff_in_effect(BuffIds::ID_OCCUPATION_MAP_CELL);

	const auto attacked_ticket_item_id = attacked_cell->get_ticket_item_id();
	if(attacked_ticket_item_id){
		// 掠夺资源。
		if(is_occupied){
			goto _plunder_done;
		}

		attacked_cell->pump_production();

		const auto amount_total = attacked_cell->get_resource_amount();
		const auto amount_to_harvest = static_cast<std::uint64_t>(std::floor(
			static_cast<double>(soldiers_damaged) / soldiers_previous * amount_total + 0.001));
		const auto amount_harvested = attacked_cell->harvest(attacking_object, amount_to_harvest, false);
		LOG_EMPERY_CENTER_DEBUG("Plunder: attacking_object_uuid = ", attacking_object_uuid,
			", attacking_object_type_id = ", attacking_object_type_id, ", attacked_coord = ", attacked_cell->get_coord(),
			", amount_to_harvest = ", amount_to_harvest, ", amount_harvested = ", amount_harvested);
	}
_plunder_done:
	;

	if(soldiers_remaining <= 0){
		if(!is_occupied){
			const auto castle_uuid = attacking_object->get_parent_object_uuid();
			const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(castle_uuid));
			if(!castle){
				goto _occupation_done;
			}
			std::vector<boost::shared_ptr<MapCell>> occupied_cells;
			WorldMap::get_map_cells_by_occupier_object(occupied_cells, castle->get_map_object_uuid());
			const auto castle_level = castle->get_level();
			const auto upgrade_data = Data::CastleUpgradePrimary::require(castle_level);
			LOG_EMPERY_CENTER_DEBUG("Try occupying map cell: attacked_coord = ", attacked_cell->get_coord(), ", castle_uuid = ", castle_uuid,
				", castle_level = ", castle_level, ", max_occupied_map_cells = ", upgrade_data->max_occupied_map_cells);
			if(occupied_cells.size() < upgrade_data->max_occupied_map_cells){
				// 占领。
				const auto exclusive_minutes = Data::Global::as_unsigned(Data::Global::SLOT_MAP_CELL_OCCUPATION_DURATION);
				const auto rescue_minutes    = Data::Global::as_unsigned(Data::Global::SLOT_MAP_CELL_RESCUE_DURATION);
				const auto occupation_duration = checked_mul<std::uint64_t>(saturated_add(exclusive_minutes, rescue_minutes), 60000);
				const auto protection_duration = checked_mul<std::uint64_t>(exclusive_minutes, 60000);

				attacked_cell->set_buff(BuffIds::ID_OCCUPATION_MAP_CELL,   utc_now, occupation_duration);
				attacked_cell->set_buff(BuffIds::ID_OCCUPATION_PROTECTION, utc_now, protection_duration);
				attacked_cell->set_occupier_object(castle);
				goto _occupation_done;
			}
		}
		// 占领失败或被解救。
		const auto protection_minutes = Data::Global::as_unsigned(Data::Global::SLOT_MAP_CELL_PROTECTION_DURATION);
		const auto protection_duration = checked_mul<std::uint64_t>(protection_minutes, 60000);

		attacked_cell->set_buff(BuffIds::ID_OCCUPATION_PROTECTION, protection_duration);
		attacked_cell->clear_buff(BuffIds::ID_OCCUPATION_MAP_CELL);
		attacked_cell->set_occupier_object({ });
	}
_occupation_done:
	;

	const auto battle_status_timeout = get_config<std::uint64_t>("battle_status_timeout", 10000);
	attacking_object->set_buff(BuffIds::ID_BATTLE_STATUS, utc_now, battle_status_timeout);
	attacked_cell->set_buff(BuffIds::ID_BATTLE_STATUS, utc_now, battle_status_timeout);

	const auto should_send_battle_notifications = !attacked_cell->is_buff_in_effect(BuffIds::ID_BATTLE_NOTIFICATION_TIMEOUT);
	attacked_cell->set_buff(BuffIds::ID_BATTLE_NOTIFICATION_TIMEOUT, utc_now, battle_status_timeout);

	// 通知客户端。
	try {
		PROFILE_ME;

		Msg:: SC_MapObjectAttackMapCellResult msg;
		msg.attacking_object_uuid   = attacking_object_uuid.str();
		msg.attacking_coord_x       = attacking_coord.x();
		msg.attacking_coord_y       = attacking_coord.y();
		msg.attacked_ticket_item_id = attacked_ticket_item_id.get();
		msg.attacked_coord_x        = attacked_coord.x();
		msg.attacked_coord_y        = attacked_coord.y();
		msg.soldiers_damaged        = soldiers_damaged;
		msg.soldiers_remaining      = soldiers_remaining;
		LOG_EMPERY_CENTER_TRACE("Broadcasting map cell attack result message: msg = ", msg);

		const auto range_left   = std::min(attacking_coord.x(), attacked_coord.x());
		const auto range_right  = std::max(attacking_coord.x(), attacked_coord.x());
		const auto range_bottom = std::min(attacking_coord.y(), attacked_coord.y());
		const auto range_top    = std::max(attacking_coord.y(), attacked_coord.y());
		std::vector<boost::shared_ptr<PlayerSession>> sessions;
		WorldMap::get_players_viewing_rectangle(sessions,
			Rectangle(Coord(range_left, range_bottom), Coord(range_right + 1, range_top + 1)));
		for(auto it = sessions.begin(); it != sessions.end(); ++it){
			const auto &session = *it;
			try {
				session->send(msg);
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			}
		}
	} catch(std::exception &e){
		LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
	}

	// 战斗通知。
	if(should_send_battle_notifications){
		const int type = (soldiers_remaining > 0) ? NOTIFY_ATTACK_MAP_CELL : NOTIFY_CAPTURE_MAP_CELL;

		if(attacking_account_uuid){
			try {
				PROFILE_ME;

				send_battle_notification(attacking_account_uuid, type, attacked_account_uuid, attacking_coord,
					attacking_object_type_id, attacked_ticket_item_id);
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
			}
		}
		if(attacked_account_uuid){
			try {
				PROFILE_ME;

				send_battle_notification(attacked_account_uuid, -type, attacking_account_uuid, attacked_coord,
					attacking_object_type_id, attacked_ticket_item_id);
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
			}
		}
	}

	// 更新交战状态。
	try {
		PROFILE_ME;

		const auto state_persistence_duration = Data::Global::as_double(Data::Global::SLOT_WAR_STATE_PERSISTENCE_DURATION);

		WarStatusMap::set(attacking_account_uuid, attacked_account_uuid,
			saturated_add(utc_now, static_cast<std::uint64_t>(state_persistence_duration * 60000)));
	} catch(std::exception &e){
		LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
	}

	return Response();
}

}
