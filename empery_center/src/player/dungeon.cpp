#include "../precompiled.hpp"
#include "common.hpp"
#include "../mmain.hpp"
#include "../msg/cs_dungeon.hpp"
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

	dungeon_box->pump_status();

	const auto prerequisite_dungeon_type_id = dungeon_data->prerequisite_dungeon_type_id;
	if(prerequisite_dungeon_type_id){
		const auto prerequisite_info = dungeon_box->get(prerequisite_dungeon_type_id);
		if(prerequisite_info.score == DungeonBox::S_NONE){
			return Response(Msg::ERR_DUNGEON_PREREQUISITE_NOT_MET) <<prerequisite_dungeon_type_id;
		}
	}
	if(!dungeon_data->reentrant){
		const auto info = dungeon_box->get(dungeon_type_id);
		if(info.score != DungeonBox::S_NONE){
			return Response(Msg::ERR_DUNGEON_DISPOSED) <<dungeon_type_id;
		}
	}
	if(req.battalions.empty()){
		return Response(Msg::ERR_DUNGEON_NO_BATTALIONS);
	}
	const auto battalion_count_limit = dungeon_data->battalion_count_limit;
	if(req.battalions.size() > battalion_count_limit){
		return Response(Msg::ERR_DUNGEON_TOO_MANY_BATTALIONS) <<battalion_count_limit;
	}
	std::vector<boost::shared_ptr<MapObject>> battalions;
	battalions.reserve(battalion_count_limit);
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
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction, true,
		[&]{
			// TODO
			auto info = dungeon_box->get(dungeon_type_id);
			info.score = DungeonBox::S_TWO_STARS;
			info.entry_count += 1;
			info.finish_count += 1;
			dungeon_box->set(std::move(info));
		});
	if(insuff_item_id){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<insuff_item_id;
	}

	return Response();
}

}
