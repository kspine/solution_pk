#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/ks_map.hpp"
#include "../msg/sk_map.hpp"
#include "../msg/sc_map.hpp"
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
#include "../castle_utilities.hpp"
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

namespace EmperyCenter {

CLUSTER_SERVLET(Msg::KS_MapRegisterCluster, cluster, req){
	const auto center_rectangle = WorldMap::get_cluster_scope(Coord(0, 0));
	const auto map_width  = static_cast<std::uint32_t>(center_rectangle.width());
	const auto map_height = static_cast<std::uint32_t>(center_rectangle.height());

	const auto num_coord = Coord(req.numerical_x, req.numerical_y);
	const auto inf_x = static_cast<std::int64_t>(INT64_MAX / map_width);
	const auto inf_y = static_cast<std::int64_t>(INT64_MAX / map_height);
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

	boost::container::flat_map<AttributeId, std::int64_t> modifiers;
	modifiers.reserve(req.attributes.size());
	for(auto it = req.attributes.begin(); it != req.attributes.end(); ++it){
		modifiers.emplace(AttributeId(it->attribute_id), it->value);
	}
	map_object->set_attributes(modifiers);

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
	const auto soldier_count = static_cast<std::uint64_t>(std::max<std::int64_t>(map_object->get_attribute(AttributeIds::ID_SOLDIER_COUNT), 0));
	const auto resource_capacity = static_cast<std::uint64_t>(map_object_type_data->resource_carriable * soldier_count);
	const auto resource_carried = map_object->get_resource_amount_carried();
	if(resource_carried >= resource_capacity){
		return Response(Msg::ERR_CARRIABLE_RESOURCE_LIMIT_EXCEEDED) <<resource_capacity;
	}

	const auto interval = req.interval;
	const auto harvested_amount = overlay->harvest(map_object, interval, true);
	LOG_EMPERY_CENTER_DEBUG("Harvest: map_object_uuid = ", map_object_uuid, ", map_object_type_id = ", map_object_type_id,
		", harvest_speed = ", harvest_speed, ", interval = ", interval, ", harvested_amount = ", harvested_amount);

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

	const auto map_object_uuid_head = Poseidon::load_be(reinterpret_cast<const std::uint64_t &>(map_object_uuid.get()[0]));

	std::vector<ResourceTransactionElement> transaction;
	boost::container::flat_map<AttributeId, std::int64_t> attributes, modifiers;
	map_object->get_attributes(attributes);
	for(auto it = attributes.begin(); it != attributes.end(); ++it){
		const auto attribute_id = it->first;
		const auto value = it->second;
		if(value <= 0){
			continue;
		}
		const auto resource_data = Data::CastleResource::get_by_carried_attribute_id(attribute_id);
		if(!resource_data){
			continue;
		}
		transaction.emplace_back(ResourceTransactionElement::OP_ADD, resource_data->resource_id, static_cast<std::uint64_t>(value),
			ReasonIds::ID_BATTALION_UNLOAD, map_object_uuid_head, 0, 0);
		modifiers.emplace(attribute_id, 0);
	}
	castle->commit_resource_transaction(transaction,
		[&]{ map_object->set_attributes(std::move(modifiers)); });

	map_object->set_garrisoned(true);

	return Response();
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
	const auto soldier_count = static_cast<std::uint64_t>(std::max<std::int64_t>(map_object->get_attribute(AttributeIds::ID_SOLDIER_COUNT), 0));
	const auto resource_capacity = static_cast<std::uint64_t>(map_object_type_data->resource_carriable * soldier_count);
	const auto resource_carried = map_object->get_resource_amount_carried();
	if(resource_carried >= resource_capacity){
		return Response(Msg::ERR_CARRIABLE_RESOURCE_LIMIT_EXCEEDED) <<resource_capacity;
	}

	const auto interval = req.interval;
	const auto harvested_amount = strategic_resource->harvest(map_object, interval, true);
	LOG_EMPERY_CENTER_DEBUG("Harvest: map_object_uuid = ", map_object_uuid, ", map_object_type_id = ", map_object_type_id,
		", harvest_speed = ", harvest_speed, ", interval = ", interval, ", harvested_amount = ", harvested_amount);

	return Response();
}

CLUSTER_SERVLET(Msg::KS_MapObjectAttackAction, cluster, req){
	const auto attacking_account_uuid   = AccountUuid(req.attacking_account_uuid);
	const auto attacking_object_uuid    = MapObjectUuid(req.attacking_object_uuid);
	const auto attacking_object_type_id = MapObjectTypeId(req.attacking_object_type_id);
	const auto attacking_coord          = Coord(req.attacking_coord_x, req.attacking_coord_y);

	const auto attacked_account_uuid    = AccountUuid(req.attacked_account_uuid);
	const auto attacked_object_uuid     = MapObjectUuid(req.attacked_object_uuid);
	const auto attacked_object_type_id  = MapObjectTypeId(req.attacked_object_type_id);
	const auto attacked_coord           = Coord(req.attacked_coord_x, req.attacked_coord_y);

	const auto result_type              = req.result_type;
	const auto result_param1            = req.result_param1;
	const auto result_param2            = req.result_param2;
	const auto soldiers_damaged         = req.soldiers_damaged;
	const auto soldiers_remaining       = req.soldiers_remaining;

	const auto utc_now = Poseidon::get_utc_time();

#define ENQUEU_JOB_SWALLOWING_EXCEPTIONS(func_)	\
	[&]{	\
		try {	\
			Poseidon::enqueue_async_job(func_);	\
		} catch(std::exception &e){	\
			LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());	\
		}	\
	}()

	// 通知客户端。
	const auto notify_clients = [=]{
		PROFILE_ME;

		Msg::SC_MapObjectAttackResult msg;
		msg.attacking_object_uuid = attacking_object_uuid.str();
		msg.attacking_coord_x     = attacking_coord.x();
		msg.attacking_coord_y     = attacking_coord.y();
		msg.attacked_object_uuid  = attacked_object_uuid.str();
		msg.attacked_coord_x      = attacked_coord.x();
		msg.attacked_coord_y      = attacked_coord.y();
		msg.result_type           = result_type;
		msg.result_param1         = result_param1;
		msg.result_param2         = result_param2;
		msg.soldiers_damaged      = soldiers_damaged;
		msg.soldiers_remaining    = soldiers_remaining;
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
	};
	ENQUEU_JOB_SWALLOWING_EXCEPTIONS(notify_clients);

	// 更新交战状态。
	const auto update_war_status = [=]{
		PROFILE_ME;

		const auto state_persistence_duration = Data::Global::as_double(Data::Global::SLOT_WAR_STATE_PERSISTENCE_DURATION);
		const auto utc_now = Poseidon::get_utc_time();

		WarStatusMap::set(attacking_account_uuid, attacked_account_uuid,
			saturated_add(utc_now, static_cast<std::uint64_t>(state_persistence_duration * 60000)));
	};
	ENQUEU_JOB_SWALLOWING_EXCEPTIONS(update_war_status);

	// 战报。
	if(attacking_account_uuid){
		const auto create_attacking_record = [=]{
			PROFILE_ME;

			const auto battle_record_box = BattleRecordBoxMap::require(attacking_account_uuid);
			battle_record_box->push(utc_now, attacking_object_type_id, attacking_coord,
				attacked_account_uuid, attacked_object_type_id, attacked_coord,
				result_type, result_param1, result_param2, soldiers_damaged, soldiers_remaining);
		};
		ENQUEU_JOB_SWALLOWING_EXCEPTIONS(create_attacking_record);
	}
	if(attacked_account_uuid){
		const auto create_attacked_record = [=]{
			PROFILE_ME;

			const auto battle_record_box = BattleRecordBoxMap::require(attacked_account_uuid);
			battle_record_box->push(utc_now, attacked_object_type_id, attacked_coord,
				attacking_account_uuid, attacking_object_type_id, attacking_coord,
				-result_type, result_param1, result_param2, soldiers_damaged, soldiers_remaining);
		};
		ENQUEU_JOB_SWALLOWING_EXCEPTIONS(create_attacked_record);
	}

	// 怪物掉落。
	if(attacking_account_uuid && (soldiers_remaining == 0)){
		const auto send_monster_reward = [=]{
			PROFILE_ME;

			const auto monster_type_data = Data::MapObjectTypeMonster::get(attacked_object_type_id);
			if(!monster_type_data){
				return;
			}

			const auto item_box = ItemBoxMap::require(attacking_account_uuid);

			const auto utc_now = Poseidon::get_utc_time();

			std::vector<ItemTransactionElement> transaction;
			transaction.reserve(16);
			const auto push_monster_rewards = [&](const boost::container::flat_map<std::string, std::uint64_t> &monster_rewards,
				ReasonId reason, std::int64_t param3)
			{
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
							transaction.emplace_back(ItemTransactionElement::OP_ADD, item_id, count,
								reason, attacked_object_type_id.get(), static_cast<std::int64_t>(reward_data->unique_id), param3);
						}
					}
				}
			};

			push_monster_rewards(monster_type_data->monster_rewards, ReasonIds::ID_MONSTER_REWARD, 0);

			std::vector<boost::shared_ptr<const Data::MapObjectTypeMonsterRewardExtra>> extra_rewards;
			Data::MapObjectTypeMonsterRewardExtra::get_available(extra_rewards, utc_now, attacked_object_type_id);
			for(auto it = extra_rewards.begin(); it != extra_rewards.end(); ++it){
				const auto &extra_reward_data = *it;
				push_monster_rewards(extra_reward_data->monster_rewards, ReasonIds::ID_MONSTER_REWARD_EXTRA, 0);
			}

			item_box->commit_transaction(transaction, false);
		};
		ENQUEU_JOB_SWALLOWING_EXCEPTIONS(send_monster_reward);
	}

	// 任务。
	if(attacking_account_uuid && (soldiers_remaining == 0)){
		const auto check_mission = [=]{
			PROFILE_ME;

			const auto attacking_object = WorldMap::get_map_object(attacking_object_uuid);
			if(!attacking_object){
				LOG_EMPERY_CENTER_DEBUG("Attacking map object is gone: attacking_object_uuid = ", attacking_object_uuid);
				return;
			}

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
		};
		ENQUEU_JOB_SWALLOWING_EXCEPTIONS(check_mission);
	}

	return Response();
}

}
