#include "../precompiled.hpp"
#include "common.hpp"
#include "../mmain.hpp"
#include "../msg/ds_dungeon.hpp"
#include "../msg/sc_dungeon.hpp"
#include "../msg/err_dungeon.hpp"
#include "../msg/err_account.hpp"
#include "../dungeon_object.hpp"
#include "../msg/err_map.hpp"
#include <poseidon/async_job.hpp>
#include "../attribute_ids.hpp"
#include "../data/map_object_type.hpp"
#include "../data/dungeon.hpp"
#include "../singletons/war_status_map.hpp"
#include "../singletons/world_map.hpp"
#include "../map_object.hpp"
#include "../castle.hpp"
#include "../transaction_element.hpp"
#include "../reason_ids.hpp"
#include "../data/global.hpp"
#include "../buff_ids.hpp"
#include "../singletons/player_session_map.hpp"
#include "../player_session.hpp"
#include "../singletons/item_box_map.hpp"
#include "../item_box.hpp"
#include "../singletons/task_box_map.hpp"
#include "../task_box.hpp"
#include "../task_type_ids.hpp"
#include "../singletons/dungeon_box_map.hpp"
#include "../dungeon_box.hpp"
#include "../singletons/item_box_map.hpp"
#include "../item_box.hpp"
#include "../reason_ids.hpp"
#include "../transaction_element.hpp"
#include "../dungeon_buff.hpp"
#include "../data/dungeon_buff.hpp"
#include <poseidon/singletons/job_dispatcher.hpp>
#include "../events/dungeon.hpp"


namespace EmperyCenter {

namespace {
	std::pair<long, std::string> DungeonRegisterServerServlet(const boost::shared_ptr<DungeonSession> &server, Poseidon::StreamBuffer payload){
		PROFILE_ME;
		Msg::DS_DungeonRegisterServer req(payload);
		LOG_EMPERY_CENTER_TRACE("Received request from ", server->get_remote_info(), ": ", req);
// ============================================================================
{
	(void)req;

	DungeonMap::add_server(server);

	return Response();
}
// ============================================================================
	}
	MODULE_RAII(handles){
		handles.push(DungeonSession::create_servlet(Msg::DS_DungeonRegisterServer::ID, &DungeonRegisterServerServlet));
	}
}

DUNGEON_SERVLET(Msg::DS_DungeonUpdateObjectAction, dungeon, server, req){
	const auto dungeon_object_uuid = DungeonObjectUuid(req.dungeon_object_uuid);
	const auto dungeon_object = dungeon->get_object(dungeon_object_uuid);
	if(!dungeon_object){
		return Response(Msg::ERR_NO_SUCH_DUNGEON_OBJECT) <<dungeon_object_uuid;
	}

	// const auto old_coord = dungeon_object->get_coord();
	const auto new_coord = Coord(req.x, req.y);
	dungeon_object->set_coord_no_synchronize(new_coord); // noexcept
	dungeon_object->set_action(req.action, std::move(req.param));

	return Response();
}

DUNGEON_SERVLET(Msg::DS_DungeonObjectAttackAction, dungeon, server, req){
	const auto attacking_object_uuid = DungeonObjectUuid(req.attacking_object_uuid);
	const auto attacked_object_uuid = DungeonObjectUuid(req.attacked_object_uuid);

	// 结算战斗伤害。
	auto attacking_object = dungeon->get_object(attacking_object_uuid);
	if(!attacking_object || attacking_object->is_virtually_removed()){
		// return Response(Msg::ERR_NO_SUCH_DUNGEON_OBJECT) <<attacking_object_uuid;
		attacking_object.reset();
	}
	const auto attacked_object = dungeon->get_object(attacked_object_uuid);
	if(!attacked_object || attacked_object->is_virtually_removed()){
		return Response(Msg::ERR_NO_SUCH_DUNGEON_OBJECT) <<attacked_object_uuid;
	}

	const auto attacking_object_type_id = attacking_object ? attacking_object->get_map_object_type_id() : MapObjectTypeId();
	const auto attacking_account_uuid = attacking_object ? attacking_object->get_owner_uuid() : AccountUuid();
	const auto attacking_coord = attacking_object ? attacking_object->get_coord() : Coord(0, 0);

	const auto attacked_object_type_id = attacked_object->get_map_object_type_id();
	const auto attacked_account_uuid = attacked_object->get_owner_uuid();
	const auto attacked_coord = attacked_object->get_coord();

	if((attacking_account_uuid == attacked_account_uuid)&&(attacking_account_uuid != AccountUuid())){
		return Response(Msg::ERR_CANNOT_ATTACK_FRIENDLY_OBJECTS);
	}

	const auto dungeon_type_id = dungeon->get_dungeon_type_id();
	const auto dungeon_data = Data::Dungeon::require(dungeon_type_id);
	const auto utc_now = Poseidon::get_utc_time();

	if(attacking_object){
		attacking_object->recalculate_attributes(false);
	}
	attacked_object->recalculate_attributes(false);

	const auto result_type = req.result_type;
	const auto hp_previous = static_cast<std::uint64_t>(attacked_object->get_attribute(AttributeIds::ID_HP_TOTAL));
	const auto hp_damaged = std::min(hp_previous, req.soldiers_damaged);
	const auto hp_remaining = checked_sub(hp_previous, hp_damaged);

	std::uint64_t hp_per_soldier = 1;
	const auto attacked_type_data = Data::MapObjectTypeAbstract::get(attacked_object_type_id);
	if(attacked_type_data){
		hp_per_soldier = std::max<std::uint64_t>(attacked_type_data->hp_per_soldier, 1);
	}
	const auto soldiers_previous = static_cast<std::uint64_t>(std::ceil(static_cast<double>(hp_previous) / hp_per_soldier - 0.001));
	const auto soldiers_remaining = static_cast<std::uint64_t>(std::ceil(static_cast<double>(hp_remaining) / hp_per_soldier - 0.001));
	const auto soldiers_damaged = saturated_sub(soldiers_previous, soldiers_remaining);
	LOG_EMPERY_CENTER_DEBUG("Dungeon object damaged: attacked_object_uuid = ", attacked_object_uuid,
		", hp_previous = ", hp_previous, ", hp_damaged = ", hp_damaged, ", hp_remaining = ", hp_remaining,
		", soldiers_previous = ", soldiers_previous, ", soldiers_damaged = ", soldiers_damaged, ", soldiers_remaining = ", soldiers_remaining);

	// 通知客户端。
	try {
		PROFILE_ME;

		Msg::SC_DungeonObjectAttackResult msg;
		msg.dungeon_uuid             = dungeon->get_dungeon_uuid().str();
		msg.attacking_object_uuid    = attacking_object_uuid.str();
		msg.attacking_coord_x        = attacking_coord.x();
		msg.attacking_coord_y        = attacking_coord.y();
		msg.attacked_object_uuid     = attacked_object_uuid.str();
		msg.attacked_coord_x         = attacked_coord.x();
		msg.attacked_coord_y         = attacked_coord.y();
		msg.result_type              = result_type;
		msg.soldiers_resuscitated    = 0; // FIXME soldiers_resuscitated;
		msg.soldiers_wounded         = 0; // FIXME soldiers_wounded;
		msg.soldiers_wounded_added   = 0; // FIXME soldiers_wounded_added;
		msg.soldiers_damaged         = hp_damaged;
		msg.soldiers_remaining       = hp_remaining;
		msg.attacking_object_type_id = attacking_object_type_id.get();
		msg.attacked_object_type_id  = attacked_object_type_id.get();
		LOG_EMPERY_CENTER_TRACE("Broadcasting attack result message: msg = ", msg);

		std::vector<std::pair<AccountUuid, boost::shared_ptr<PlayerSession>>> observers_all;
		dungeon->get_observers_all(observers_all);
		for(auto it = observers_all.begin(); it != observers_all.end(); ++it){
			const auto &session = it->second;
			try {
				session->send(msg);
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			}
		}
	} catch(std::exception &e){
		LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
	}

	const auto shadow_attacked_map_object_uuid = MapObjectUuid(attacked_object_uuid.get());
	const auto shadow_attacked_map_object = WorldMap::get_map_object(shadow_attacked_map_object_uuid);

	boost::container::flat_map<AttributeId, std::int64_t> modifiers;
	modifiers[AttributeIds::ID_SOLDIER_COUNT] = static_cast<std::int64_t>(soldiers_remaining);
	modifiers[AttributeIds::ID_HP_TOTAL]      = static_cast<std::int64_t>(hp_remaining);
	if(shadow_attacked_map_object){
		shadow_attacked_map_object->set_attributes(modifiers);
	}
	attacked_object->set_attributes(std::move(modifiers));

	if(soldiers_remaining <= 0){
		LOG_EMPERY_CENTER_DEBUG("Map object is dead now: attacked_object_uuid = ", attacked_object_uuid);
		if(shadow_attacked_map_object){
			shadow_attacked_map_object->delete_from_game();
		}
		attacked_object->delete_from_game();
	}

	// 回收伤兵。
	std::uint64_t soldiers_resuscitated = 0;
	std::uint64_t soldiers_wounded = 0, soldiers_wounded_added = 0;
	{
		PROFILE_ME;

		const auto attacked_type_data = Data::MapObjectTypeBattalion::get(attacked_object_type_id);
		if(!attacked_type_data){
			goto _wounded_done;
		}
		if(attacked_type_data->speed <= 0){
			goto _wounded_done;
		}

		if(!shadow_attacked_map_object){
			goto _wounded_done;
		}
		const auto parent_object_uuid = shadow_attacked_map_object->get_parent_object_uuid();
		if(!parent_object_uuid){
			goto _wounded_done;
		}
		const auto parent_castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(parent_object_uuid));
		if(!parent_castle){
			LOG_EMPERY_CENTER_WARNING("No such castle: parent_object_uuid = ", parent_object_uuid);
			goto _wounded_done;
		}

		const auto attacked_object_uuid_head = Poseidon::load_be(reinterpret_cast<const std::uint64_t &>(attacked_object_uuid.get()[0]));

		// 先算复活的。
		const auto resuscitation_ratio = dungeon_data->resuscitation_ratio;
		soldiers_resuscitated = static_cast<std::uint64_t>(soldiers_damaged * resuscitation_ratio + 0.001);
		if(soldiers_resuscitated > 0){
			std::vector<SoldierTransactionElement> soldier_transaction;
			soldier_transaction.emplace_back(SoldierTransactionElement::OP_ADD, attacked_object_type_id, soldiers_resuscitated,
				ReasonIds::ID_DUNGEON_RESUSCITATION, attacked_object_uuid_head, soldiers_damaged, std::round(soldiers_resuscitated * 1000));
			parent_castle->commit_soldier_transaction(soldier_transaction);
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
		soldiers_wounded = static_cast<std::uint64_t>(saturated_sub(soldiers_damaged, soldiers_resuscitated) * wounded_ratio + 0.001);
		LOG_EMPERY_CENTER_DEBUG("Wounded soldiers: wounded_ratio_basic = ", wounded_ratio_basic, ", wounded_ratio_bonus = ", wounded_ratio_bonus,
			", soldiers_damaged = ", soldiers_damaged, ", soldiers_wounded = ", soldiers_wounded);

		std::uint64_t capacity_used = 0;
		std::vector<Castle::WoundedSoldierInfo> wounded_soldier_all;
		parent_castle->get_wounded_soldiers_all(wounded_soldier_all);
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

		std::vector<WoundedSoldierTransactionElement> wounded_soldier_transaction;
		wounded_soldier_transaction.emplace_back(WoundedSoldierTransactionElement::OP_ADD, attacked_object_type_id, soldiers_wounded_added,
			ReasonIds::ID_SOLDIER_WOUNDED, attacked_object_uuid_head, soldiers_damaged, std::round(wounded_ratio_bonus * 1000));
		parent_castle->commit_wounded_soldier_transaction(wounded_soldier_transaction);
	}
_wounded_done:
	dungeon->update_soldier_stat(attacked_account_uuid, attacked_object_type_id,
		soldiers_damaged, soldiers_resuscitated, soldiers_wounded_added);

	const auto battle_status_timeout = get_config<std::uint64_t>("battle_status_timeout", 10000);
	if(attacking_object){
		attacking_object->set_buff(BuffIds::ID_BATTLE_STATUS, utc_now, battle_status_timeout);
	}
	attacked_object->set_buff(BuffIds::ID_BATTLE_STATUS, utc_now, battle_status_timeout);

	// 更新交战状态。
	try {
		PROFILE_ME;

		const auto state_persistence_duration = Data::Global::as_double(Data::Global::SLOT_WAR_STATE_PERSISTENCE_DURATION);

		WarStatusMap::set(attacking_account_uuid, attacked_account_uuid,
		saturated_add(utc_now, static_cast<std::uint64_t>(state_persistence_duration * 60000)));
	} catch(std::exception &e){
		LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
	}

	// 怪物掉落。
	if(attacking_account_uuid && (soldiers_remaining == 0)){
		try {
			Poseidon::enqueue_async_job([=]() mutable {
				PROFILE_ME;

				const auto monster_type_data = Data::MapObjectTypeDungeonMonster::get(attacked_object_type_id);
				if(!monster_type_data){
					return;
				}

				const auto item_box = ItemBoxMap::get(attacking_account_uuid);
				if(!item_box){
					LOG_EMPERY_CENTER_DEBUG("Failed to load item box: attacking_account_uuid = ", attacking_account_uuid);
					return;
				}

				const auto shadow_attacking_map_object_uuid = MapObjectUuid(attacking_object_uuid.get());
				const auto shadow_attacking_map_object = WorldMap::get_map_object(shadow_attacking_map_object_uuid);
				if(!shadow_attacking_map_object){
					return;
				}
				const auto parent_object_uuid = shadow_attacking_map_object->get_parent_object_uuid();
				if(!parent_object_uuid){
					return;
				}
				const auto parent_castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(parent_object_uuid));
				if(!parent_castle){
					LOG_EMPERY_CENTER_WARNING("No such castle: parent_object_uuid = ", parent_object_uuid);
					return;
				}

				boost::container::flat_map<ItemId, std::uint64_t> items_basic;

				{
					std::vector<ItemTransactionElement> transaction;
					const auto &monster_rewards = monster_type_data->monster_rewards;
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
									ReasonIds::ID_DUNGEON_MONSTER_REWARD, attacked_object_type_id.get(),
									static_cast<std::int64_t>(reward_data->unique_id), 0);
								items_basic[item_id] += count;
							}
						}
					}

					item_box->commit_transaction(transaction, false);
				}

				const auto session = PlayerSessionMap::get(attacking_account_uuid);
				if(session){
					try {
						Msg::SC_DungeonMonsterRewardGot msg;
						msg.dungeon_uuid       = dungeon->get_dungeon_uuid().str();
						msg.x                  = attacked_coord.x();
						msg.y                  = attacked_coord.y();
						msg.map_object_type_id = attacked_object_type_id.get();
						msg.items_basic.reserve(items_basic.size());
						for(auto it = items_basic.begin(); it != items_basic.end(); ++it){
							auto &elem = *msg.items_basic.emplace(msg.items_basic.end());
							elem.item_id = it->first.get();
							elem.count   = it->second;
						}
						msg.castle_uuid        = parent_castle->get_map_object_uuid().str();
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
			Poseidon::enqueue_async_job([=]() mutable {
				PROFILE_ME;

				const auto task_box = TaskBoxMap::get(attacking_account_uuid);
				if(!task_box){
					LOG_EMPERY_CENTER_DEBUG("Failed to load task box: attacking_account_uuid = ", attacking_account_uuid);
					return;
				}

				const auto primary_castle = WorldMap::require_primary_castle(attacking_account_uuid);
				const auto primary_castle_uuid = primary_castle->get_map_object_uuid();

				auto task_type_id = TaskTypeIds::ID_WIPE_OUT_MONSTERS;
				if(attacked_account_uuid){
					task_type_id = TaskTypeIds::ID_WIPE_OUT_ENEMY_BATTALIONS;
				}
				auto castle_category = TaskBox::TCC_PRIMARY;
				MapObjectUuid parent_object_uuid;
				const auto shadow_attacking_map_object_uuid = MapObjectUuid(attacking_object_uuid.get());
				const auto shadow_attacking_map_object = WorldMap::get_map_object(shadow_attacking_map_object_uuid);
				if(shadow_attacking_map_object){
					parent_object_uuid = shadow_attacking_map_object->get_parent_object_uuid();
				}
				if(parent_object_uuid != primary_castle_uuid){
					castle_category = TaskBox::TCC_NON_PRIMARY;
				}
				task_box->check(task_type_id, attacked_object_type_id.get(), 1,
					castle_category, 0, 0);
			});
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
		}
	}
	return Response();
}

DUNGEON_SERVLET(Msg::DS_DungeonSetScope, dungeon, server, req){
	const auto scope = Rectangle(req.x, req.y, req.width, req.height);

	dungeon->set_scope(scope);

	return Response();
}

DUNGEON_SERVLET(Msg::DS_DungeonCreateMonster, dungeon, server, req){
	const auto map_object_type_id = MapObjectTypeId(req.map_object_type_id);
	const auto monster_data = Data::MapObjectTypeDungeonMonster::get(map_object_type_id);
	if(!monster_data){
		return Response(Msg::ERR_NO_SUCH_DUNGEON_OBJECT_TYPE) <<map_object_type_id;
	}

	const auto monster_uuid = DungeonObjectUuid(Poseidon::Uuid::random());
	const auto coord = Coord(req.x, req.y);
	const auto dest_coord = Coord(req.dest_x,req.dest_y);

	const auto soldier_count = monster_data->max_soldier_count;
	const auto hp_total = checked_mul(monster_data->max_soldier_count, monster_data->hp_per_soldier);

	boost::container::flat_map<AttributeId, std::int64_t> modifiers;
	modifiers.reserve(8);
	modifiers[AttributeIds::ID_SOLDIER_COUNT]               = static_cast<std::int64_t>(soldier_count);
	modifiers[AttributeIds::ID_SOLDIER_COUNT_MAX]           = static_cast<std::int64_t>(soldier_count);
	modifiers[AttributeIds::ID_MONSTER_START_POINT_X]       = coord.x();
	modifiers[AttributeIds::ID_MONSTER_START_POINT_Y]       = coord.y();
	modifiers[AttributeIds::ID_MONSTER_PATROL_DEST_POINT_X] = dest_coord.x();
	modifiers[AttributeIds::ID_MONSTER_PATROL_DEST_POINT_Y]      = dest_coord.y();
	modifiers[AttributeIds::ID_HP_TOTAL]                    = static_cast<std::int64_t>(hp_total);

	auto dungeon_object = boost::make_shared<DungeonObject>(dungeon->get_dungeon_uuid(), monster_uuid,
		map_object_type_id, AccountUuid(), std::move(req.tag), coord);
	dungeon_object->set_attributes(std::move(modifiers));
	dungeon_object->pump_status();

	dungeon->insert_object(std::move(dungeon_object));

	return Response();
}

DUNGEON_SERVLET(Msg::DS_DungeonWaitForPlayerConfirmation, dungeon, server, req){
	const auto &suspension = dungeon->get_suspension();
	if(suspension.type != 0){
		return Response(Msg::ERR_DUNGEON_SUSPENDED);
	}

	Msg::SC_DungeonWaitForPlayerConfirmation msg;
	msg.dungeon_uuid = dungeon->get_dungeon_uuid().str();
	msg.context      = std::move(req.context);
	msg.type         = req.type;
	msg.param1       = req.param1;
	msg.param2       = req.param2;
	msg.param3       = std::move(req.param3);
	dungeon->broadcast_to_observers(msg);

	return Response();
}

DUNGEON_SERVLET(Msg::DS_DungeonObjectStopped, dungeon, server, req){
	const auto dungeon_object_uuid = DungeonObjectUuid(req.dungeon_object_uuid);
	const auto dungeon_object = dungeon->get_object(dungeon_object_uuid);
	if(!dungeon_object){
		return Response(Msg::ERR_NO_SUCH_DUNGEON_OBJECT) <<dungeon_object_uuid;
	}

	const auto owner_uuid = dungeon_object->get_owner_uuid();
	const auto session = dungeon->get_observer(owner_uuid);
	if(session){
		try {
			Msg::SC_DungeonObjectStopped msg;
			msg.dungeon_uuid        = dungeon->get_dungeon_uuid().str();
			msg.dungeon_object_uuid = dungeon_object_uuid.str();
			msg.action              = req.action;
			msg.param               = req.param;
			msg.error_code          = req.error_code;
			msg.error_message       = std::move(req.error_message);
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}

	return Response();
}

DUNGEON_SERVLET(Msg::DS_DungeonWaypointsSet, dungeon, server, req){
	const auto dungeon_object_uuid = DungeonObjectUuid(req.dungeon_object_uuid);
	const auto dungeon_object = dungeon->get_object(dungeon_object_uuid);
	if(!dungeon_object){
		return Response(Msg::ERR_NO_SUCH_DUNGEON_OBJECT) <<dungeon_object_uuid;
	}

	const auto owner_uuid = dungeon_object->get_owner_uuid();
	const auto session = dungeon->get_observer(owner_uuid);
	if(session){
		try {
			Msg::SC_DungeonWaypointsSet msg;
			msg.dungeon_uuid        = dungeon->get_dungeon_uuid().str();
			msg.dungeon_object_uuid = dungeon_object_uuid.str();
			msg.x                   = req.x;
			msg.y                   = req.y;
			msg.waypoints.reserve(req.waypoints.size());
			for(auto it = req.waypoints.begin(); it != req.waypoints.end(); ++it){
				auto &elem = *msg.waypoints.emplace(msg.waypoints.end());
				elem.dx = it->dx;
				elem.dy = it->dy;
			}
			msg.action              = req.action;
			msg.param               = std::move(req.param);
			msg.target_x            = req.target_x;
			msg.target_y            = req.target_y;
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}

	return Response();
}

DUNGEON_SERVLET(Msg::DS_DungeonPlayerWins, dungeon, server, req){
	const auto account_uuid = AccountUuid(req.account_uuid);
	const auto dungeon_box = DungeonBoxMap::get(account_uuid);
	if(!dungeon_box){
		return Response(Msg::ERR_CONTROLLER_TOKEN_NOT_ACQUIRED);
	}

	const auto item_box = ItemBoxMap::require(account_uuid);

	boost::container::flat_map<ItemId, std::uint64_t> rewards;
	boost::container::flat_map<DungeonTaskId, boost::container::flat_map<ItemId, std::uint64_t>> tasks_new;

	std::vector<ItemTransactionElement> transaction;
	transaction.reserve(32);

	const auto dungeon_type_id = dungeon->get_dungeon_type_id();

	auto info = dungeon_box->get(dungeon_type_id);
	info.finish_count += 1;

	const auto dungeon_data = Data::Dungeon::require(dungeon_type_id);
	for(auto it = dungeon_data->rewards.begin(); it != dungeon_data->rewards.end(); ++it){
		const auto item_id = it->first;
		const auto count = it->second;
		transaction.emplace_back(ItemTransactionElement::OP_ADD, item_id, count,
			ReasonIds::ID_FINISH_DUNGEON_TASK, dungeon_type_id.get(), 0, 0);
		rewards[item_id] += count;
	}

	for(auto tit = req.tasks_finished.begin(); tit != req.tasks_finished.end(); ++tit){
		const auto dungeon_task_id = DungeonTaskId(tit->dungeon_task_id);
		if(dungeon_data->tasks.find(dungeon_task_id) == dungeon_data->tasks.end()){
			LOG_EMPERY_CENTER_WARNING("Dungeon task ignored: dungeon_task_id = ", dungeon_task_id, ", dungeon_type_id = ", dungeon_type_id);
			continue;
		}
		if(info.tasks_finished.insert(dungeon_task_id).second){
			const auto task_data = Data::DungeonTask::require(dungeon_task_id);
			for(auto it = task_data->rewards.begin(); it != task_data->rewards.end(); ++it){
				const auto item_id = it->first;
				const auto count = it->second;
				transaction.emplace_back(ItemTransactionElement::OP_ADD, item_id, count,
					ReasonIds::ID_FINISH_DUNGEON_TASK, dungeon_type_id.get(), dungeon_task_id.get(), 0);
				tasks_new[dungeon_task_id][item_id] += count;
			}
		}
	}

	item_box->commit_transaction(transaction, false,
		[&]{ dungeon_box->set(std::move(info)); });

	const auto session = PlayerSessionMap::get(account_uuid);
	if(session){
		try {
			Msg::SC_DungeonFinished msg;
			msg.dungeon_uuid    = dungeon->get_dungeon_uuid().str();
			msg.dungeon_type_id = dungeon->get_dungeon_type_id().get();
			msg.rewards.reserve(rewards.size());
			for(auto it = rewards.begin(); it != rewards.end(); ++it){
				auto &elem = *msg.rewards.emplace(msg.rewards.end());
				elem.item_id = it->first.get();
				elem.count   = it->second;
			}
			msg.tasks_finished_new.reserve(tasks_new.size());
			for(auto it = tasks_new.begin(); it != tasks_new.end(); ++it){
				auto &task_elem = *msg.tasks_finished_new.emplace(msg.tasks_finished_new.end());
				task_elem.dungeon_task_id = it->first.get();
				task_elem.rewards.reserve(it->second.size());
				for(auto tit = it->second.begin(); tit != it->second.end(); ++tit){
					auto &reward_elem = *task_elem.rewards.emplace(task_elem.rewards.end());
					reward_elem.item_id = tit->first.get();
					reward_elem.count   = tit->second;
				}
			}
			std::vector<std::pair<MapObjectTypeId, Dungeon::SoldierStat>> soldier_stats;
			dungeon->get_soldier_stats(soldier_stats, account_uuid);
			for(auto it = soldier_stats.begin(); it != soldier_stats.end(); ++it){
				auto &soldier_elem = *msg.soldier_stats.emplace(msg.soldier_stats.end());
				soldier_elem.map_object_type_id = it->first.get();
				soldier_elem.soldiers_damaged = it->second.damaged;
				soldier_elem.soldiers_resuscitated = it->second.resuscitated;
				soldier_elem.soldiers_wounded = it->second.wounded;
			}
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}

	//副本通关任务
	const auto task_box = TaskBoxMap::require(account_uuid);
    task_box->check_task_dungeon_clearance(boost::lexical_cast<uint64_t>(dungeon_type_id),info.finish_count);

	dungeon->remove_observer(account_uuid, Dungeon::Q_PLAYER_WINS, "");
	const auto utc_now = Poseidon::get_utc_time();
	LOG_EMPERY_CENTER_FATAL(req);
	auto event = boost::make_shared<Events::DungeonFinish>(account_uuid,dungeon->get_dungeon_type_id(),dungeon->get_create_time(),utc_now,true);
	Poseidon::async_raise_event(event);
	DungeonMap::remove(dungeon);
	return Response();
}

DUNGEON_SERVLET(Msg::DS_DungeonPlayerLoses, dungeon, server, req){
	const auto account_uuid = AccountUuid(req.account_uuid);
	const auto dungeon_box = DungeonBoxMap::get(account_uuid);
	if(!dungeon_box){
		return Response(Msg::ERR_CONTROLLER_TOKEN_NOT_ACQUIRED);
	}
	const auto session = PlayerSessionMap::get(account_uuid);
	if(session){
		try {
			Msg::SC_DungeonFailed msg;
			msg.dungeon_uuid    = dungeon->get_dungeon_uuid().str();
			msg.dungeon_type_id = dungeon->get_dungeon_type_id().get();
			std::vector<std::pair<MapObjectTypeId, Dungeon::SoldierStat>> soldier_stats;
			dungeon->get_soldier_stats(soldier_stats, account_uuid);
			for(auto it = soldier_stats.begin(); it != soldier_stats.end(); ++it){
				auto &soldier_elem = *msg.soldier_stats.emplace(msg.soldier_stats.end());
				soldier_elem.map_object_type_id = it->first.get();
				soldier_elem.soldiers_damaged = it->second.damaged;
				soldier_elem.soldiers_resuscitated = it->second.resuscitated;
				soldier_elem.soldiers_wounded = it->second.wounded;
			}
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
	dungeon->remove_observer(account_uuid, Dungeon::Q_PLAYER_LOSES, "");
	const auto utc_now = Poseidon::get_utc_time();
	auto event = boost::make_shared<Events::DungeonFinish>(account_uuid,dungeon->get_dungeon_type_id(),dungeon->get_create_time(),utc_now,false);
	Poseidon::async_raise_event(event);
	DungeonMap::remove(dungeon);
	return Response();
}

DUNGEON_SERVLET(Msg::DS_DungeonMoveCamera, dungeon, server, req){
	Msg::SC_DungeonMoveCamera msg;
	msg.dungeon_uuid      = dungeon->get_dungeon_uuid().str();
	msg.x                 = req.x;
	msg.y                 = req.y;
	msg.movement_duration = req.movement_duration;
	msg.position_type     = req.position_type;
	dungeon->broadcast_to_observers(msg);

	return Response();
}

DUNGEON_SERVLET(Msg::DS_DungeonTriggerEffectForcast, dungeon, server, req){
	Msg::SC_DungeonTriggerEffectForcast msg;
	msg.dungeon_uuid      = dungeon->get_dungeon_uuid().str();
	msg.trigger_id        = req.trigger_id;
	msg.executive_time    = req.executive_time;
	for(auto it = req.effects.begin(); it != req.effects.end(); ++it){
		auto &effects = *msg.effects.emplace(msg.effects.end());
		effects.effect_type = it->effect_type;
	}
	dungeon->broadcast_to_observers(msg);

	return Response();
}

DUNGEON_SERVLET(Msg::DS_DungeonTriggerEffectExecutive, dungeon, server, req){
	Msg::SC_DungeonTriggerEffectExecutive msg;
	msg.dungeon_uuid      = dungeon->get_dungeon_uuid().str();
	msg.trigger_id        = req.trigger_id;
	for(auto it = req.effects.begin(); it != req.effects.end(); ++it){
		auto &effects = *msg.effects.emplace(msg.effects.end());
		effects.effect_type = it->effect_type;
	}
	dungeon->broadcast_to_observers(msg);

	return Response();
}

DUNGEON_SERVLET(Msg::DS_DungeonShowPicture, dungeon, server, req){
	Msg::SC_DungeonShowPicture msg;
	msg.dungeon_uuid      = dungeon->get_dungeon_uuid().str();
	msg.picture_url       = req.picture_url;
	msg.picture_id        = req.picture_id;
	msg.type              = req.type;
	msg.layer             = req.layer;
	msg.tween             = req.tween;
	msg.time              = req.time;
	msg.x                 = req.x;
	msg.y                 = req.y;
	dungeon->broadcast_to_observers(msg);

	return Response();
}

DUNGEON_SERVLET(Msg::DS_DungeonRemovePicture, dungeon, server, req){
	Msg::SC_DungeonRemovePicture msg;
	msg.dungeon_uuid      = dungeon->get_dungeon_uuid().str();
	msg.picture_id        = req.picture_id;
	msg.tween             = req.tween;
	msg.time              = req.time;
	dungeon->broadcast_to_observers(msg);

	return Response();
}

DUNGEON_SERVLET(Msg::DS_DungeonCreateBuff, dungeon, server, req){
	const auto dungeon_buff_type_id = DungeonBuffTypeId(req.buff_type_id);
	const auto buff_data = Data::DungeonBuff::get(dungeon_buff_type_id);
	if(!buff_data){
		return Response(Msg::ERR_NO_DUNGEON_BUFF_DATA) <<dungeon_buff_type_id;
	}
	const auto coord = Coord(req.x, req.y);
	const auto utc_now = Poseidon::get_utc_time();
	const auto create_uuid = DungeonObjectUuid(req.create_uuid); 
	const auto create_owner_uuid = AccountUuid(req.create_owner_uuid);
	auto dungeon_buff = dungeon->get_dungeon_buff(coord);
	auto expired_time = utc_now + buff_data->continue_time*1000;
	auto new_dungeon_buff = boost::make_shared<DungeonBuff>(dungeon->get_dungeon_uuid(), dungeon_buff_type_id,create_uuid,create_owner_uuid,coord,expired_time);
	if(dungeon_buff){
		if(dungeon_buff->get_expired_time() < utc_now){
			LOG_EMPERY_CENTER_FATAL("insert a dungeon buff while old buff not expired at coord",coord);
		}else{
			dungeon->update_dungeon_buff(std::move(new_dungeon_buff));
		}
	}else{
		dungeon->insert_dungeon_buff(std::move(new_dungeon_buff));
	}

	return Response();
}

DUNGEON_SERVLET(Msg::DS_DungeonCreateBlocks, dungeon, server, req){
	Msg::SC_DungeonCreateBlocks msg;
	msg.dungeon_uuid      = dungeon->get_dungeon_uuid().str();
	for(auto it = req.blocks.begin(); it != req.blocks.end(); ++it){
		auto &blocks = *msg.blocks.emplace(msg.blocks.end());
		blocks.x = it->x;
		blocks.y = it->y;
	}
	dungeon->broadcast_to_observers(msg);
	return Response();
}

DUNGEON_SERVLET(Msg::DS_DungeonRemoveBlocks, dungeon, server, req){
	Msg::SC_DungeonRemoveBlocks msg;
	msg.dungeon_uuid      = dungeon->get_dungeon_uuid().str();
	for(auto it = req.blocks.begin(); it != req.blocks.end(); ++it){
		auto &blocks = *msg.blocks.emplace(msg.blocks.end());
		blocks.x = it->x;
		blocks.y = it->y;
	}
	dungeon->broadcast_to_observers(msg);

	return Response();
}

DUNGEON_SERVLET(Msg::DS_DungeonHideSolider, dungeon, server, req){
	Msg::SC_DungeonHideSolider msg;
	msg.dungeon_uuid      = dungeon->get_dungeon_uuid().str();
	msg.type              = req.type;
	dungeon->broadcast_to_observers(msg);
	return Response();
}

DUNGEON_SERVLET(Msg::DS_DungeonUnhideSolider, dungeon, server, req){
	Msg::SC_DungeonUnhideSolider msg;
	msg.dungeon_uuid      = dungeon->get_dungeon_uuid().str();
	msg.type              = req.type;
	dungeon->broadcast_to_observers(msg);
	return Response();
}

DUNGEON_SERVLET(Msg::DS_DungeonHideCoords, dungeon, server, req){
	Msg::SC_DungeonHideCoords msg;
	msg.dungeon_uuid      = dungeon->get_dungeon_uuid().str();
	for(auto it = req.hide_coord.begin(); it != req.hide_coord.end(); ++it){
		auto &hide_coord = *msg.hide_coord.emplace(msg.hide_coord.end());
		hide_coord.x = it->x;
		hide_coord.y = it->y;
	}
	dungeon->broadcast_to_observers(msg);

	return Response();
}

DUNGEON_SERVLET(Msg::DS_DungeonUnhideCoords, dungeon, server, req){
	Msg::SC_DungeonUnhideCoords msg;
	msg.dungeon_uuid      = dungeon->get_dungeon_uuid().str();
	for(auto it = req.unhide_coord.begin(); it != req.unhide_coord.end(); ++it){
		auto &unhide_coord = *msg.unhide_coord.emplace(msg.unhide_coord.end());
		unhide_coord.x = it->x;
		unhide_coord.y = it->y;
	}
	dungeon->broadcast_to_observers(msg);

	return Response();
}

DUNGEON_SERVLET(Msg::DS_DungeonObjectSkillSingAction, dungeon, server, req){
	const auto attacking_object_uuid = DungeonObjectUuid(req.attacking_object_uuid);

	auto attacking_object = dungeon->get_object(attacking_object_uuid);
	if(!attacking_object || attacking_object->is_virtually_removed()){
		attacking_object.reset();
	}

	Msg::SC_DungeonObjectSkillSingAction msg;
	msg.dungeon_uuid               = req.dungeon_uuid;
	msg.attacking_account_uuid     = req.attacking_account_uuid;
	msg.attacking_object_uuid      = req.attacking_object_uuid;
	msg.attacking_object_type_id   = req.attacking_object_type_id;
	msg.attacking_coord_x          = req.attacking_coord_x;
	msg.attacking_coord_y          = req.attacking_coord_y;
    msg.attacked_coord_x           = req.attacked_coord_x;
	msg.attacked_coord_y           = req.attacked_coord_y;
	msg.skill_type_id              = req.skill_type_id;
	msg.sing_delay                 = req.sing_delay;

	dungeon->broadcast_to_observers(msg);
	return Response();
}

DUNGEON_SERVLET(Msg::DS_DungeonObjectSkillCastAction, dungeon, server, req){
	const auto attacking_object_uuid = DungeonObjectUuid(req.attacking_object_uuid);

	auto attacking_object = dungeon->get_object(attacking_object_uuid);
	if(!attacking_object || attacking_object->is_virtually_removed()){
		attacking_object.reset();
	}

	Msg::SC_DungeonObjectSkillCastAction msg;
	msg.dungeon_uuid               = req.dungeon_uuid;
	msg.attacking_account_uuid     = req.attacking_account_uuid;
	msg.attacking_object_uuid      = req.attacking_object_uuid;
	msg.attacking_object_type_id   = req.attacking_object_type_id;
	msg.attacking_coord_x          = req.attacking_coord_x;
	msg.attacking_coord_y          = req.attacking_coord_y;
    msg.attacked_coord_x           = req.attacked_coord_x;
	msg.attacked_coord_y           = req.attacked_coord_y;
	msg.skill_type_id              = req.skill_type_id;
	msg.cast_delay                 = req.cast_delay;
	dungeon->broadcast_to_observers(msg);
	return Response();
}

DUNGEON_SERVLET(Msg::DS_DungeonObjectSkillEffect, dungeon, server, req){
	const auto attacking_object_uuid = DungeonObjectUuid(req.attacking_object_uuid);

	auto attacking_object = dungeon->get_object(attacking_object_uuid);
	if(!attacking_object || attacking_object->is_virtually_removed()){
		attacking_object.reset();
	}
	Msg::SC_DungeonObjectSkillEffect msg;
	msg.dungeon_uuid               = req.dungeon_uuid;
	msg.attacking_account_uuid     = req.attacking_account_uuid;
	msg.attacking_object_uuid      = req.attacking_object_uuid;
	msg.attacking_object_type_id   = req.attacking_object_type_id;
	msg.attacking_coord_x          = req.attacking_coord_x;
	msg.attacking_coord_y          = req.attacking_coord_y;
    msg.attacked_coord_x           = req.attacked_coord_x;
	msg.attacked_coord_y           = req.attacked_coord_y;
	msg.skill_type_id              = req.skill_type_id;
	for(unsigned i = 0; i < req.effect_range.size(); ++i){
		auto &effect_coord = req.effect_range.at(i);
		auto &range_coord   = *msg.effect_range.emplace(msg.effect_range.end());
		range_coord.x = effect_coord.x;
		range_coord.y = effect_coord.y;
	}
	dungeon->broadcast_to_observers(msg);
	return Response();
}

DUNGEON_SERVLET(Msg::DS_DungeonObjectPrepareTransmit, dungeon, server, req){
	Msg::SC_DungeonObjectPrepareTransmit msg;
	msg.dungeon_uuid               = req.dungeon_uuid;
	for(unsigned i = 0; i < req.transmit_objects.size(); ++i){
		auto &req_transmit_object = req.transmit_objects.at(i);
		auto &transmit_object  = *msg.transmit_objects.emplace(msg.transmit_objects.end());
		transmit_object.object_uuid = req_transmit_object.object_uuid;
		transmit_object.x = req_transmit_object.x;
		transmit_object.y = req_transmit_object.y;
	}
	dungeon->broadcast_to_observers(msg);
	return Response();
}

DUNGEON_SERVLET(Msg::DS_DungeonObjectAddBuff, dungeon, server, req){
	const auto dungeon_buff_type_id = DungeonBuffTypeId(req.buff_type_id);
	const auto dungeon_object_uuid         = DungeonObjectUuid(req.dungeon_object_uuid);
	const auto buff_data = Data::DungeonBuff::get(dungeon_buff_type_id);
	if(!buff_data){
		return Response(Msg::ERR_NO_DUNGEON_BUFF_DATA) <<dungeon_buff_type_id;
	}
	auto dungeon_object = dungeon->get_object(dungeon_object_uuid);
	if(!dungeon_object){
		return Response(Msg::ERR_NO_SUCH_DUNGEON_OBJECT) <<dungeon_object_uuid;
	}
	auto continue_time = buff_data->continue_time*1000;
	dungeon_object->set_buff(BuffId(dungeon_buff_type_id.get()),continue_time);
	return Response();
}

DUNGEON_SERVLET(Msg::DS_DungeonSetFootAnnimation, dungeon, server, req){
	Msg::SC_DungeonSetFootAnnimation msg;
	msg.dungeon_uuid      = dungeon->get_dungeon_uuid().str();
	msg.picture_url       = req.picture_url;
	msg.type              = req.type;
	msg.x                 = req.x;
	msg.y                 = req.y;
	msg.layer             = req.layer;
	for(unsigned i = 0; i < req.monsters.size(); ++i){
		auto &req_monster = req.monsters.at(i);
		auto &monster = *msg.monsters.emplace(msg.monsters.end());
		monster.monster_uuid = req_monster.monster_uuid;
	}
	dungeon->broadcast_to_observers(msg);

	return Response();
}

}
