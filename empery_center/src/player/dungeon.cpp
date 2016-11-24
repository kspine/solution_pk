#include "../precompiled.hpp"
#include "common.hpp"
#include "../mmain.hpp"
#include "../msg/cs_dungeon.hpp"
#include "../msg/sc_dungeon.hpp"
#include "../msg/sd_dungeon.hpp"
#include "../msg/err_dungeon.hpp"
#include "../msg/err_item.hpp"
#include "../msg/err_map.hpp"
#include "../singletons/dungeon_box_map.hpp"
#include "../dungeon_box.hpp"
#include "../data/dungeon.hpp"
#include "../singletons/item_box_map.hpp"
#include "../item_box.hpp"
#include "../transaction_element.hpp"
#include "../reason_ids.hpp"
#include "../singletons/world_map.hpp"
#include "../data/map_defense.hpp"
#include "../data/map_object_type.hpp"
#include "../defense_building.hpp"
#include "../attribute_ids.hpp"
#include "../singletons/dungeon_map.hpp"
#include "../dungeon.hpp"
#include "../dungeon_object.hpp"
#include "../msg/kill.hpp"
#include "../dungeon_session.hpp"
#include "../map_utilities.hpp"
#include <poseidon/singletons/job_dispatcher.hpp>
#include "../events/dungeon.hpp"
#include "../task_box.hpp"
#include "../singletons/task_box_map.hpp"
namespace EmperyCenter {

PLAYER_SERVLET(Msg::CS_DungeonGetAll, account, session, /* req */){
	const auto dungeon_box = DungeonBoxMap::require(account->get_account_uuid());
	dungeon_box->pump_status();

	dungeon_box->synchronize_with_player(session);

	return Response();
}

PLAYER_SERVLET(Msg::CS_DungeonCreate, account, session, req){
	const auto dungeon_type_id = DungeonTypeId(req.dungeon_type_id);
	const auto dungeon_data = Data::Dungeon::get(dungeon_type_id);
	if(!dungeon_data){
		return Response(Msg::ERR_NO_SUCH_DUNGEON_ID) <<dungeon_type_id;
	}
	const auto account_uuid = account->get_account_uuid();

	const auto dungeon_box = DungeonBoxMap::require(account_uuid);
	const auto item_box = ItemBoxMap::require(account_uuid);
	const auto task_box = TaskBoxMap::require(account_uuid);

	dungeon_box->pump_status();

	const auto prerequisite_dungeon_type_id = dungeon_data->prerequisite_dungeon_type_id;
	if(prerequisite_dungeon_type_id){
		const auto prerequisite_info = dungeon_box->get(prerequisite_dungeon_type_id);
		if(prerequisite_info.finish_count == 0){
			return Response(Msg::ERR_DUNGEON_PREREQUISITE_NOT_MET) <<prerequisite_dungeon_type_id;
		}
	}
	for(auto it = dungeon_data->need_tasks.begin(); it != dungeon_data->need_tasks.end(); ++it){
		auto task_id = *it;
		if(!task_box->check_reward_status(TaskId(task_id.get()))){
			return Response(Msg::ERR_DUNGEON_NEED_TASK_NOT_REWARD) << task_id;
		}
	}

	auto info = dungeon_box->get(dungeon_type_id);
	if(!dungeon_data->reentrant){
		if(info.finish_count != 0){
			return Response(Msg::ERR_DUNGEON_DISPOSED) <<dungeon_type_id;
		}
	}

	const auto server = DungeonMap::pick_server();
	if(!server){
		return Response(Msg::ERR_NO_DUNGEON_SERVER_AVAILABLE);
	}

	if(req.battalions.empty()){
		return Response(Msg::ERR_DUNGEON_NO_BATTALIONS);
	}
	const auto &start_points = dungeon_data->start_points;
	if(req.battalions.size() > start_points.size()){
		return Response(Msg::ERR_DUNGEON_TOO_MANY_BATTALIONS) <<start_points.size();
	}
	std::vector<boost::shared_ptr<MapObject>> battalions;
	battalions.reserve(start_points.size());
	for(auto it = req.battalions.begin(); it != req.battalions.end(); ++it){
		const auto map_object_uuid = MapObjectUuid(it->map_object_uuid);
		auto map_object = WorldMap::get_map_object(map_object_uuid);
		if(!map_object){
			return Response(Msg::ERR_NO_SUCH_MAP_OBJECT) <<map_object_uuid;
		}
		if(map_object->get_owner_uuid() != account->get_account_uuid()){
			return Response(Msg::ERR_NOT_YOUR_MAP_OBJECT) <<map_object->get_owner_uuid();
		}

		const auto soldier_count = map_object->get_attribute(AttributeIds::ID_SOLDIER_COUNT);
		if(soldier_count <= 0){
			continue;
		}
		if(static_cast<std::uint64_t>(soldier_count) < dungeon_data->soldier_count_limits.first){
			return Response(Msg::ERR_DUNGEON_TOO_FEW_SOLDIERS) <<map_object_uuid;
		}
		if(static_cast<std::uint64_t>(soldier_count) >= dungeon_data->soldier_count_limits.second){
			return Response(Msg::ERR_DUNGEON_TOO_MANY_SOLDIERS) <<map_object_uuid;
		}

		const auto does_type_match = [&](const std::pair<const Data::ApplicabilityKeyType, std::uint64_t> &key){
			if(key.first == Data::AKT_ALL){
				return true;
			}
			const auto map_object_type_id = map_object->get_map_object_type_id();
			if(key.first == Data::AKT_TYPE_ID){
				return key.second == map_object_type_id.get();
			}
			// XXX: optimize this somehow.
			const auto defense = boost::dynamic_pointer_cast<DefenseBuilding>(map_object);
			if(defense){
				const auto defense_data = Data::MapDefenseBuildingAbstract::get(map_object_type_id, defense->get_level());
				if(defense_data){
					if(key.first == Data::AKT_WEAPON_ID){
						const auto combat_data = Data::MapDefenseCombat::require(defense_data->defense_combat_id);
						return key.second == combat_data->map_object_weapon_id.get();
					}
				}
			} else {
				const auto battalion_data = Data::MapObjectTypeBattalion::get(map_object_type_id);
				if(battalion_data){
					if(key.first == Data::AKT_LEVEL_ID){
						return key.second == battalion_data->battalion_level_id.get();
					}
				}
				auto map_object_data = boost::shared_ptr<const Data::MapObjectTypeAbstract>(battalion_data);
				if(!map_object_data){
					map_object_data = Data::MapObjectTypeAbstract::get(map_object_type_id);
				}
				if(map_object_data){
					if(key.first == Data::AKT_CHASSIS_ID){
						return key.second == map_object_data->map_object_chassis_id.get();
					}
					if(key.first == Data::AKT_WEAPON_ID){
						return key.second == map_object_data->map_object_weapon_id.get();
					}
				}
			}
			return false;
		};
		if(!dungeon_data->battalions_required.empty() &&
			std::none_of(dungeon_data->battalions_required.begin(), dungeon_data->battalions_required.end(), does_type_match))
		{
			return Response(Msg::ERR_BATTALION_TYPE_FORBIDDEN) <<map_object_uuid;
		}
		if(!dungeon_data->battalions_forbidden.empty() &&
			std::any_of(dungeon_data->battalions_forbidden.begin(), dungeon_data->battalions_forbidden.end(), does_type_match))
		{
			return Response(Msg::ERR_BATTALION_TYPE_FORBIDDEN) <<map_object_uuid;
		}

		battalions.emplace_back(std::move(map_object));
	}

	const auto &entry_cost = dungeon_data->entry_cost;
	std::vector<ItemTransactionElement> transaction;
	transaction.reserve(entry_cost.size());
	for(auto it = entry_cost.begin(); it != entry_cost.end(); ++it){
		transaction.emplace_back(ItemTransactionElement::OP_REMOVE, it->first, it->second,
			ReasonIds::ID_CREATE_DUNGEON, dungeon_type_id.get(), 0, 0);
	}

	const auto dungeon_uuid = DungeonUuid(Poseidon::Uuid::random());
	const auto utc_now = Poseidon::get_utc_time();

	const auto expiry_duration = get_config<std::uint64_t>("dungeon_expiry_duration", 900000);
	const auto expiry_time = saturated_add(utc_now, expiry_duration);

	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction, true,
		[&]{
			const auto dungeon = boost::make_shared<Dungeon>(dungeon_uuid, dungeon_type_id, server, account_uuid,utc_now, expiry_time,info.finish_count);
			dungeon->insert_observer(account_uuid, session);
			for(std::size_t i = 0; i < battalions.size(); ++i){
				const auto &map_object = battalions.at(i);
				const auto map_object_uuid = map_object->get_map_object_uuid();
				const auto map_object_type_id = map_object->get_map_object_type_id();

				const auto &start_point = start_points.at(i);
				const auto start_coord = Coord(start_point.first, start_point.second);
				LOG_EMPERY_CENTER_DEBUG("@@ New dungeon object: dungeon_uuid = ", dungeon_uuid,
					", map_object_uuid = ", map_object_uuid, ", start_coord = ", start_coord);

				auto dungeon_object = boost::make_shared<DungeonObject>(dungeon_uuid, DungeonObjectUuid(map_object_uuid.get()),
					map_object_type_id, account_uuid, std::string(), start_coord);
				dungeon_object->pump_status();
				dungeon_object->recalculate_attributes(false);
				dungeon->insert_object(std::move(dungeon_object));
			}
			DungeonMap::insert(std::move(dungeon));

			info.entry_count += 1;
			dungeon_box->set(std::move(info));
			auto  event = boost::make_shared<Events::DungeonCreated>(
					account_uuid, dungeon_type_id);
			Poseidon::async_raise_event(event);
		});
	if(insuff_item_id){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<insuff_item_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_DungeonQuit, account, session, req){
	const auto dungeon_uuid = DungeonUuid(req.dungeon_uuid);
	const auto dungeon = DungeonMap::get(dungeon_uuid);
	if(!dungeon){
		return Response(Msg::ERR_NO_SUCH_DUNGEON) <<dungeon_uuid;
	}

	const auto account_uuid = account->get_account_uuid();
	const auto observer_session = dungeon->get_observer(account_uuid);
	if(observer_session != session){
		return Response(Msg::ERR_NOT_IN_DUNGEON) <<dungeon_uuid;
	}
	const auto utc_now = Poseidon::get_utc_time();
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
	dungeon->remove_observer(account_uuid, Dungeon::Q_PLAYER_REQUEST, { });
	auto event = boost::make_shared<Events::DungeonFinish>(account_uuid,dungeon->get_dungeon_type_id(),dungeon->get_create_time(),utc_now,false);
	Poseidon::async_raise_event(event);
	DungeonMap::remove(dungeon);
	return Response();
}

PLAYER_SERVLET(Msg::CS_DungeonSetWaypoints, account, session, req){
	const auto dungeon_uuid = DungeonUuid(req.dungeon_uuid);
	const auto dungeon = DungeonMap::get(dungeon_uuid);
	if(!dungeon){
		return Response(Msg::ERR_NO_SUCH_DUNGEON) <<dungeon_uuid;
	}

	const auto server = dungeon->get_server();
	if(!server){
		return Response(Msg::ERR_DUNGEON_SERVER_CONNECTION_LOST);
	}

	const auto dungeon_object_uuid = DungeonObjectUuid(req.dungeon_object_uuid);
	const auto dungeon_object = dungeon->get_object(dungeon_object_uuid);
	if(!dungeon_object){
		return Response(Msg::ERR_NO_SUCH_DUNGEON_OBJECT) <<dungeon_object_uuid;
	}
	if(dungeon_object->get_owner_uuid() != account->get_account_uuid()){
		return Response(Msg::ERR_NOT_YOUR_DUNGEON_OBJECT) <<dungeon_object->get_owner_uuid();
	}

	auto old_coord = dungeon_object->get_coord();

	Msg::SD_DungeonSetAction dreq;
	dreq.dungeon_uuid        = dungeon_uuid.str();
	dreq.dungeon_object_uuid = dungeon_object_uuid.str();
	dreq.x                   = old_coord.x();
	dreq.y                   = old_coord.y();
	// 撤销当前的路径。
	auto dresult = server->send_and_wait(dreq);
	if(dresult.first != Msg::ST_OK){
		LOG_EMPERY_CENTER_WARNING("Dungeon server returned an error: code = ", dresult.first, ", msg = ", dresult.second);
		// return std::move(dresult);
		server->shutdown(Msg::KILL_DUNGEON_SERVER_RESYNCHRONIZE, "Lost dungeon synchronization");
		return Response(Msg::ERR_DUNGEON_SERVER_CONNECTION_LOST);
	}

	dungeon_object->recalculate_attributes(false);

	// 重新计算坐标。
	old_coord = dungeon_object->get_coord();
	dreq.x    = old_coord.x();
	dreq.y    = old_coord.y();
	dreq.waypoints.reserve(std::min<std::size_t>(req.waypoints.size(), 256));
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

		auto &waypoint = *dreq.waypoints.emplace(dreq.waypoints.end());
		waypoint.dx = step.dx;
		waypoint.dy = step.dy;
	}
	dreq.action = req.action;
	dreq.param  = std::move(req.param);
	dresult = server->send_and_wait(dreq);
	if(dresult.first != Msg::ST_OK){
		LOG_EMPERY_CENTER_DEBUG("Cluster server returned an error: code = ", dresult.first, ", msg = ", dresult.second);
		return std::move(dresult);
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_DungeonStopTroops, account, session, req){
	const auto dungeon_uuid = DungeonUuid(req.dungeon_uuid);
	const auto dungeon = DungeonMap::get(dungeon_uuid);
	if(!dungeon){
		return Response(Msg::ERR_NO_SUCH_DUNGEON) <<dungeon_uuid;
	}

	const auto server = dungeon->get_server();
	if(!server){
		return Response(Msg::ERR_DUNGEON_SERVER_CONNECTION_LOST);
	}

	Msg::SC_DungeonStopTroopsRet msg;
	msg.dungeon_uuid = dungeon_uuid.str();
	msg.dungeon_objects.reserve(std::min<std::size_t>(req.dungeon_objects.size(), 16));
	for(auto it = req.dungeon_objects.begin(); it != req.dungeon_objects.end(); ++it){
		const auto dungeon_object_uuid = DungeonObjectUuid(it->dungeon_object_uuid);
		const auto dungeon_object = dungeon->get_object(dungeon_object_uuid);
		if(!dungeon_object){
			LOG_EMPERY_CENTER_DEBUG("No such dungeon object: dungeon_uuid = ", dungeon_uuid, ", dungeon_object_uuid = ", dungeon_object_uuid);
			continue;
		}
		if(dungeon_object->get_owner_uuid() != account->get_account_uuid()){
			LOG_EMPERY_CENTER_DEBUG("Not your object: dungeon_object_uuid = ", dungeon_object_uuid);
			continue;
		}

		auto old_coord = dungeon_object->get_coord();

		Msg::SD_DungeonSetAction dreq;
		dreq.dungeon_uuid        = dungeon_uuid.str();
		dreq.dungeon_object_uuid = dungeon_object_uuid.str();
		dreq.x                   = old_coord.x();
		dreq.y                   = old_coord.y();
		// 撤销当前的路径。
		const auto dresult = server->send_and_wait(dreq);
		if(dresult.first != Msg::ST_OK){
			LOG_EMPERY_CENTER_WARNING("Dungeon server returned an error: code = ", dresult.first, ", msg = ", dresult.second);
			server->shutdown(Msg::KILL_DUNGEON_SERVER_RESYNCHRONIZE, "Lost dungeon synchronization");
			continue;
		}

		auto &elem = *msg.dungeon_objects.emplace(msg.dungeon_objects.end());
		elem.dungeon_object_uuid = dungeon_object_uuid.str();
	}
	session->send(msg);

	return Response();
}

PLAYER_SERVLET(Msg::CS_DungeonPlayerConfirmation, account, session, req){
	const auto dungeon_uuid = DungeonUuid(req.dungeon_uuid);
	const auto dungeon = DungeonMap::get(dungeon_uuid);
	if(!dungeon){
		return Response(Msg::ERR_NO_SUCH_DUNGEON) <<dungeon_uuid;
	}

	const auto server = dungeon->get_server();
	if(!server){
		return Response(Msg::ERR_DUNGEON_SERVER_CONNECTION_LOST);
	}

	Msg::SD_DungeonPlayerConfirmation dreq;
	dreq.dungeon_uuid = dungeon_uuid.str();
	dreq.context      = std::move(req.context);
	auto dresult = server->send_and_wait(dreq);
	if(dresult.first != Msg::ST_OK){
		LOG_EMPERY_CENTER_DEBUG("Dungeon server returned an error: code = ", dresult.first, ", msg = ", dresult.second);
		return std::move(dresult);
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_DungeonBegin, account, session, req){
	const auto dungeon_uuid = DungeonUuid(req.dungeon_uuid);
	const auto dungeon = DungeonMap::get(dungeon_uuid);
	if(!dungeon){
		return Response(Msg::ERR_NO_SUCH_DUNGEON) <<dungeon_uuid;
	}

	const auto account_uuid = account->get_account_uuid();
	const auto observer_session = dungeon->get_observer(account_uuid);
	if(observer_session != session){
		return Response(Msg::ERR_NOT_IN_DUNGEON) <<dungeon_uuid;
	}
	if(dungeon->is_begin()){
		LOG_EMPERY_CENTER_WARNING("dungeon already begin .... ");
		return Response(Msg::ERR_DUNGEON_ALREADY_BEGIN) <<dungeon_uuid;
	}
	const auto server = dungeon->get_server();
	if(!server){
		return Response(Msg::ERR_DUNGEON_SERVER_CONNECTION_LOST);
	}
	Msg::SD_DungeonBegin dreq;
	dreq.dungeon_uuid        = dungeon_uuid.str();
	const auto dresult = server->send_and_wait(dreq);
	if(dresult.first != Msg::ST_OK){
		LOG_EMPERY_CENTER_WARNING("Dungeon server returned an error: code = ", dresult.first, ", msg = ", dresult.second);
		return std::move(dresult);
	}
	dungeon->set_begin(true);
	return Response();
}

}
