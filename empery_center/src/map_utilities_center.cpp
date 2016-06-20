#include "precompiled.hpp"
#include "map_utilities_center.hpp"
#include "map_utilities.hpp"
#include "cbpp_response.hpp"
#include "data/global.hpp"
#include "data/map.hpp"
#include "map_cell.hpp"
#include "overlay.hpp"
#include "map_object.hpp"
#include "strategic_resource.hpp"
#include "singletons/world_map.hpp"
#include "msg/err_map.hpp"
#include "msg/err_castle.hpp"
#include "msg/sc_map.hpp"
#include "map_object_type_ids.hpp"
#include "resource_crate.hpp"
#include <poseidon/json.hpp>
#include <poseidon/async_job.hpp>
#include "castle.hpp"
#include "item_box.hpp"
#include "singletons/item_box_map.hpp"
#include "transaction_element.hpp"
#include "mail_box.hpp"
#include "singletons/mail_box_map.hpp"
#include "mail_data.hpp"
#include "chat_message_type_ids.hpp"
#include "chat_message_slot_ids.hpp"
#include "map_object_type_ids.hpp"
#include "item_ids.hpp"
#include "reason_ids.hpp"
#include "data/map_object_type.hpp"
#include "attribute_ids.hpp"
#include "buff_ids.hpp"
#include "player_session.hpp"

namespace EmperyCenter {

std::pair<long, std::string> can_place_defense_building_at(Coord coord){
	PROFILE_ME;

	// 检测阻挡。
	const auto map_cell = WorldMap::get_map_cell(coord);
	if(map_cell){
		const auto cell_owner_uuid = map_cell->get_owner_uuid();
		if(cell_owner_uuid){
			LOG_EMPERY_CENTER_TRACE("Blocked by a cell owned by another player's territory: cell_owner_uuid = ", cell_owner_uuid);
			return CbppResponse(Msg::ERR_BLOCKED_BY_OTHER_TERRITORY) <<cell_owner_uuid;
		}
	}

	const auto cluster_scope = WorldMap::get_cluster_scope(coord);
	const auto map_x = static_cast<unsigned>(coord.x() - cluster_scope.left());
	const auto map_y = static_cast<unsigned>(coord.y() - cluster_scope.bottom());
	const auto cell_data = Data::MapCellBasic::require(map_x, map_y);
	const auto terrain_id = cell_data->terrain_id;
	const auto terrain_data = Data::MapTerrain::require(terrain_id);
	if(!terrain_data->passable){
		LOG_EMPERY_CENTER_TRACE("Blocked by terrain: terrain_id = ", terrain_id);
		return CbppResponse(Msg::ERR_BLOCKED_BY_IMPASSABLE_MAP_CELL) <<terrain_id;
	}
	const unsigned border_thickness = Data::Global::as_unsigned(Data::Global::SLOT_MAP_BORDER_THICKNESS);
	if((map_x < border_thickness) || (map_x >= cluster_scope.width() - border_thickness) ||
		(map_y < border_thickness) || (map_y >= cluster_scope.height() - border_thickness))
	{
		LOG_EMPERY_CENTER_TRACE("Blocked by map border: coord = ", coord);
		return CbppResponse(Msg::ERR_BLOCKED_BY_IMPASSABLE_MAP_CELL) <<coord;
	}

	std::vector<boost::shared_ptr<MapObject>> adjacent_objects;
	WorldMap::get_map_objects_by_rectangle(adjacent_objects,
		Rectangle(Coord(coord.x() - 3, coord.y() - 3), Coord(coord.x() + 4, coord.y() + 4)));
	std::vector<Coord> foundation;
	for(auto it = adjacent_objects.begin(); it != adjacent_objects.end(); ++it){
		const auto &other_object = *it;
		const auto other_map_object_uuid = other_object->get_map_object_uuid();
		const auto other_coord = other_object->get_coord();
		if(coord == other_coord){
			LOG_EMPERY_CENTER_TRACE("Blocked by another map object: other_map_object_uuid = ", other_map_object_uuid);
			return CbppResponse(Msg::ERR_BLOCKED_BY_TROOPS) <<other_map_object_uuid;
		}
		const auto other_object_type_id = other_object->get_map_object_type_id();
		if(other_object_type_id == MapObjectTypeIds::ID_CASTLE){
			foundation.clear();
			get_castle_foundation(foundation, other_coord, false);
			for(auto fit = foundation.begin(); fit != foundation.end(); ++fit){
				if(coord == *fit){
					LOG_EMPERY_CENTER_TRACE("Blocked by castle: other_map_object_uuid = ", other_map_object_uuid);
					return CbppResponse(Msg::ERR_BLOCKED_BY_CASTLE) <<other_map_object_uuid;
				}
			}
		}
	}

	std::vector<boost::shared_ptr<ResourceCrate>> resource_crates;
	WorldMap::get_resource_crates_by_rectangle(resource_crates, Rectangle(coord, 1, 1));
	for(auto it = resource_crates.begin(); it != resource_crates.end(); ++it){
		const auto &resource_crate = *it;
		if(!resource_crate->is_virtually_removed()){
			return CbppResponse(Msg::ERR_CANNOT_PLACE_DEFENSE_ON_CRATES) <<resource_crate->get_resource_crate_uuid();
		}
	}

	return CbppResponse();
}

std::pair<long, std::string> can_deploy_castle_at(Coord coord, MapObjectUuid excluding_map_object_uuid){
	PROFILE_ME;

	using Response = CbppResponse;

	std::vector<boost::shared_ptr<MapCell>> map_cells;
	std::vector<boost::shared_ptr<Overlay>> overlays;
	std::vector<boost::shared_ptr<MapObject>> map_objects;
	std::vector<boost::shared_ptr<StrategicResource>> strategic_resources;
	std::vector<boost::shared_ptr<ResourceCrate>> resource_crates;

	std::vector<Coord> foundation;
	get_castle_foundation(foundation, coord, true);
	for(auto it = foundation.begin(); it != foundation.end(); ++it){
		const auto &foundation_coord = *it;

		const auto cluster_scope = WorldMap::get_cluster_scope(foundation_coord);
		const auto map_x = static_cast<unsigned>(foundation_coord.x() - cluster_scope.left());
		const auto map_y = static_cast<unsigned>(foundation_coord.y() - cluster_scope.bottom());
		LOG_EMPERY_CENTER_DEBUG("Castle foundation: foundation_coord = ", foundation_coord, ", cluster_scope = ", cluster_scope,
			", map_x = ", map_x, ", map_y = ", map_y);
		const auto basic_data = Data::MapCellBasic::require(map_x, map_y);
		const auto terrain_data = Data::MapTerrain::require(basic_data->terrain_id);
		if(!terrain_data->buildable){
			return Response(Msg::ERR_CANNOT_DEPLOY_IMMIGRANTS_HERE) <<foundation_coord;
		}

		map_cells.clear();
		WorldMap::get_map_cells_by_rectangle(map_cells, Rectangle(foundation_coord, 1, 1));
		for(auto it = map_cells.begin(); it != map_cells.end(); ++it){
			const auto &map_cell = *it;
			if(!map_cell->is_virtually_removed()){
				return Response(Msg::ERR_CANNOT_DEPLOY_ON_TERRITORY) <<foundation_coord;
			}
		}

		overlays.clear();
		WorldMap::get_overlays_by_rectangle(overlays, Rectangle(foundation_coord, 1, 1));
		for(auto it = overlays.begin(); it != overlays.end(); ++it){
			const auto &overlay = *it;
			if(!overlay->is_virtually_removed()){
				return Response(Msg::ERR_CANNOT_DEPLOY_ON_OVERLAY) <<foundation_coord;
			}
		}

		map_objects.clear();
		WorldMap::get_map_objects_by_rectangle(map_objects, Rectangle(foundation_coord, 1, 1));
		for(auto it = map_objects.begin(); it != map_objects.end(); ++it){
			const auto &other_object = *it;
			if(other_object->get_map_object_uuid() == excluding_map_object_uuid){
				continue;
			}
			if(!other_object->is_virtually_removed()){
				return Response(Msg::ERR_CANNOT_DEPLOY_ON_TROOPS) <<foundation_coord;
			}
		}

		strategic_resources.clear();
		WorldMap::get_strategic_resources_by_rectangle(strategic_resources, Rectangle(foundation_coord, 1, 1));
		for(auto it = strategic_resources.begin(); it != strategic_resources.end(); ++it){
			const auto &strategic_resource = *it;
			if(!strategic_resource->is_virtually_removed()){
				return Response(Msg::ERR_CANNOT_DEPLOY_ON_STRATEGIC_RESOURCE) <<foundation_coord;
			}
		}
	}

	const auto solid_offset = get_castle_foundation_solid_area();
	for(auto it = foundation.begin(); it != foundation.begin() + static_cast<std::ptrdiff_t>(solid_offset); ++it){
		const auto &foundation_coord = *it;

		resource_crates.clear();
		WorldMap::get_resource_crates_by_rectangle(resource_crates, Rectangle(foundation_coord, 1, 1));
		for(auto it = resource_crates.begin(); it != resource_crates.end(); ++it){
			const auto &resource_crate = *it;
			if(!resource_crate->is_virtually_removed()){
				return Response(Msg::ERR_CANNOT_DEPLOY_ON_RESOURCE_CRATES) <<resource_crate->get_resource_crate_uuid();
			}
		}
	}

	// 检测与其他城堡距离。
	const auto min_distance  = static_cast<std::uint32_t>(Data::Global::as_unsigned(Data::Global::SLOT_MINIMUM_DISTANCE_BETWEEN_CASTLES));

	const auto cluster_scope = WorldMap::get_cluster_scope(coord);
	const auto other_left    = std::max(coord.x() - (min_distance - 1), cluster_scope.left());
	const auto other_bottom  = std::max(coord.y() - (min_distance - 1), cluster_scope.bottom());
	const auto other_right   = std::min(coord.x() + (min_distance + 2), cluster_scope.right());
	const auto other_top     = std::min(coord.y() + (min_distance + 2), cluster_scope.top());
	map_objects.clear();
	WorldMap::get_map_objects_by_rectangle(map_objects, Rectangle(Coord(other_left, other_bottom), Coord(other_right, other_top)));
	for(auto it = map_objects.begin(); it != map_objects.end(); ++it){
		const auto &other_object = *it;
		const auto other_object_type_id = other_object->get_map_object_type_id();
		if(other_object_type_id != MapObjectTypeIds::ID_CASTLE){
			continue;
		}
		const auto other_coord = other_object->get_coord();
		const auto other_object_uuid = other_object->get_map_object_uuid();
		LOG_EMPERY_CENTER_DEBUG("Checking distance: other_coord = ", other_coord, ", other_object_uuid = ", other_object_uuid);
		const auto distance = get_distance_of_coords(other_coord, coord);
		if(distance <= min_distance){
			return Response(Msg::ERR_TOO_CLOSE_TO_ANOTHER_CASTLE) <<other_object_uuid;
		}
	}

	return Response();
}

void create_resource_crates(Coord origin, ResourceId resource_id, std::uint64_t amount,
	unsigned radius_inner, unsigned radius_outer, MapObjectTypeId map_object_type_id)
{
	PROFILE_ME;
	LOG_EMPERY_CENTER_DEBUG("Creating resource crates: origin = ", origin, ", resource_id = ", resource_id, ", amount = ", amount,
		", radius_inner = ", radius_inner, ", radius_outer = ", radius_outer);

	const auto border_thickness            = Data::Global::as_unsigned(Data::Global::SLOT_MAP_BORDER_THICKNESS);
	const auto inner_amount_ratio          = Data::Global::as_double(Data::Global::SLOT_RESOURCE_CRATE_AMOUNT_INNER_RATIO);
	const auto separation_amount_threshold = Data::Global::as_unsigned(Data::Global::SLOT_RESOURCE_CRATE_SEPARATION_AMOUNT_THRESHOLD);
	const auto number_limits               = Data::Global::as_array(Data::Global::SLOT_RESOURCE_CRATE_NUMBER_LIMITS);
	const auto expiry_duration             = Data::Global::as_double(Data::Global::SLOT_RESOURCE_CRATE_RADIUS_EXPIRY_DURATION);

	const auto really_create_crates = [&](std::uint64_t &amount_remaining,
		unsigned radius_begin, unsigned radius_end, unsigned number_limit)
	{
		if(amount_remaining == 0){
			return;
		}
		if(radius_begin >= radius_end){
			return;
		}
		unsigned crate_count = 1;
		if(amount_remaining >= separation_amount_threshold){
			crate_count += Poseidon::rand32(1, number_limit);
		}
		LOG_EMPERY_CENTER_DEBUG("> amount_remaining = ", amount_remaining, ", crate_count = ", crate_count, ", number_limit = ", number_limit);
		if(crate_count == 0){
			LOG_EMPERY_CENTER_WARNING("Crate count is zero?");
			return;
		}
		const auto resource_amount_per_crate = amount_remaining / crate_count;

		std::vector<Coord> coords;
		const unsigned n_radius = radius_end - radius_begin;
		coords.reserve(n_radius * 6 + 1 + n_radius * (n_radius - 1) * 3);
		for(unsigned i = radius_begin; i < radius_end; ++i){
			get_surrounding_coords(coords, origin, i);
		}

		std::vector<boost::shared_ptr<MapObject>> adjacent_objects;
		std::vector<boost::shared_ptr<ResourceCrate>> adjacent_crates;

		std::vector<Coord> foundation;
		const auto solid_offset = get_castle_foundation_solid_area();
		coords.erase(
			std::remove_if(coords.begin(), coords.end(),
				[&](Coord coord){
					const auto cluster_scope = WorldMap::get_cluster_scope(coord);
					const auto map_x = static_cast<unsigned>(coord.x() - cluster_scope.left());
					const auto map_y = static_cast<unsigned>(coord.y() - cluster_scope.bottom());
					const auto cell_data = Data::MapCellBasic::require(map_x, map_y);
					const auto terrain_id = cell_data->terrain_id;
					const auto terrain_data = Data::MapTerrain::require(terrain_id);
					if(!terrain_data->passable){
						return true;
					}
					if((map_x < border_thickness) || (map_x >= cluster_scope.width() - border_thickness) ||
						(map_y < border_thickness) || (map_y >= cluster_scope.height() - border_thickness))
					{
						return true;
					}

					adjacent_objects.clear();
					WorldMap::get_map_objects_by_rectangle(adjacent_objects,
						Rectangle(Coord(coord.x() - 3, coord.y() - 3), Coord(coord.x() + 4, coord.y() + 4)));
					for(auto it = adjacent_objects.begin(); it != adjacent_objects.end(); ++it){
						const auto &other_object = *it;
						const auto other_object_type_id = other_object->get_map_object_type_id();
						if(other_object_type_id == MapObjectTypeIds::ID_CASTLE){
							foundation.clear();
							get_castle_foundation(foundation, other_object->get_coord(), true);
							for(auto fit = foundation.begin(); fit != foundation.begin() + static_cast<std::ptrdiff_t>(solid_offset); ++fit){
								if(*fit == coord){
									return true;
								}
							}
							continue;
						}
						const auto defense_building = boost::dynamic_pointer_cast<DefenseBuilding>(other_object);
						if(defense_building){
							if(defense_building->get_coord() == coord){
								return true;
							}
						}
					}

					adjacent_crates.clear();
					WorldMap::get_resource_crates_by_rectangle(adjacent_crates, Rectangle(coord, 1, 1));
					for(auto it = adjacent_crates.begin(); it != adjacent_crates.end(); ++it){
						const auto &other_crate = *it;
						if(other_crate->get_coord() == coord){
							return true;
						}
					}

					return false;
				}),
			coords.end());
		if(coords.empty()){
			LOG_EMPERY_CENTER_DEBUG("> No coords available.");
			return;
		}

		const auto utc_now = Poseidon::get_utc_time();

		for(unsigned i = 0; i < crate_count; ++i){
			try {
				const auto resource_crate_uuid = ResourceCrateUuid(Poseidon::Uuid::random());
				const auto coord = coords.at(Poseidon::rand32(0, coords.size()));
				const auto expiry_time = saturated_add(utc_now, static_cast<std::uint64_t>(expiry_duration * 60000));

				auto resource_crate = boost::make_shared<ResourceCrate>(resource_crate_uuid,
					resource_id, resource_amount_per_crate, coord, utc_now, expiry_time);
				WorldMap::insert_resource_crate(std::move(resource_crate));
				LOG_EMPERY_CENTER_DEBUG("> Created resource crate: resource_crate_uuid = ", resource_crate_uuid,
					", coord = ", coord, ", resource_id = ", resource_id, ", resource_amount_per_crate = ", resource_amount_per_crate);
				amount_remaining -= resource_amount_per_crate;
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			}
		}
		// 消除误差。
		if(amount_remaining < crate_count){
			amount_remaining = 0;
		}
	};

	Msg::SC_MapResourceCrateExplosion msg;
	msg.coord_x            = origin.x();
	msg.coord_y            = origin.y();
	msg.map_object_type_id = map_object_type_id.get();

	std::vector<boost::shared_ptr<PlayerSession>> sessions;
	WorldMap::get_players_viewing_rectangle(sessions, Rectangle(origin, 1, 1));

	std::uint64_t amount_remaining = amount * (1 - inner_amount_ratio);
	const auto outer_number_limit = static_cast<unsigned>(std::round(number_limits.at(1).get<double>()));
	really_create_crates(amount_remaining, radius_inner, radius_inner + radius_outer, outer_number_limit);
	LOG_EMPERY_CENTER_DEBUG("Outer crate creation complete: resource_id = ", resource_id, ", amount_remaining = ", amount_remaining);
	amount_remaining += amount * inner_amount_ratio;
	const auto inner_number_limit = static_cast<unsigned>(std::round(number_limits.at(0).get<double>()));
	really_create_crates(amount_remaining, 0, radius_inner, inner_number_limit);
	LOG_EMPERY_CENTER_DEBUG("Inner crate creation complete: resource_id = ", resource_id, ", amount_remaining = ", amount_remaining);

	for(auto it = sessions.begin(); it != sessions.end(); ++it){
		const auto &session = *it;
		try {
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
}

void async_hang_up_castle(const boost::shared_ptr<Castle> &castle) noexcept
try {
	PROFILE_ME;

	const auto really_hang_up = [=]{
		PROFILE_ME;

		const auto castle_uuid = castle->get_map_object_uuid();
		const auto castle_uuid_head = Poseidon::load_be(reinterpret_cast<const std::uint64_t &>(castle_uuid.get()[0]));
		const auto account_uuid = castle->get_owner_uuid();

		const auto item_box = ItemBoxMap::require(account_uuid);
		const auto mail_box = MailBoxMap::require(account_uuid);

		if(castle->is_garrisoned()){
			LOG_EMPERY_CENTER_DEBUG("Castle is already hung up: castle_uuid = ", castle_uuid);
			return;
		}

		constexpr auto new_coord = Coord(INT32_MAX, INT32_MAX);

		std::vector<boost::shared_ptr<MapObject>> child_objects;
		WorldMap::get_map_objects_by_parent_object(child_objects, castle_uuid);
		std::vector<boost::shared_ptr<MapCell>> map_cells;
		WorldMap::get_map_cells_by_parent_object(map_cells, castle_uuid);

		boost::container::flat_map<ItemId, std::uint64_t> items_regained;
		items_regained.reserve(32);
		std::vector<boost::shared_ptr<MapObject>> defenses_destroyed;
		defenses_destroyed.reserve(child_objects.size());

		const auto last_coord = castle->get_coord();

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
				child_object->set_coord(last_coord);
				child_object->set_garrisoned(true);
			} else {
				child_object->delete_from_game();
				defenses_destroyed.emplace_back(child_object);
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
				ReasonIds::ID_HANG_UP_CASTLE, castle_uuid_head, 0, 0);
			items_regained[ticket_item_id] += 1;
			if(map_cell->is_acceleration_card_applied()){
				transaction.emplace_back(ItemTransactionElement::OP_ADD, ItemIds::ID_ACCELERATION_CARD, 1,
					ReasonIds::ID_HANG_UP_CASTLE, castle_uuid_head, 0, 0);
				items_regained[ItemIds::ID_ACCELERATION_CARD] += 1;
			}
			item_box->commit_transaction(transaction, false,
				[&]{
					map_cell->clear_buff(BuffIds::ID_OCCUPATION_PROTECTION);
					map_cell->set_parent_object({ }, { }, { });
					map_cell->set_acceleration_card_applied(false);
					for(auto it = child_objects.begin(); it != child_objects.end(); ++it){
						const auto &child_object = *it;
						const auto map_object_type_id = child_object->get_map_object_type_id();
						if(map_object_type_id == MapObjectTypeIds::ID_CASTLE){
							continue;
						}
						child_object->set_coord(new_coord);
					}
				});
		}

		// 挂起城堡。
		boost::container::flat_map<AttributeId, std::int64_t> modifiers;
		modifiers.reserve(8);
		modifiers[AttributeIds::ID_CASTLE_LAST_COORD_X] = last_coord.x();
		modifiers[AttributeIds::ID_CASTLE_LAST_COORD_Y] = last_coord.y();
		castle->set_attributes(std::move(modifiers));

		castle->set_coord(new_coord);
		castle->set_garrisoned(true);

		for(auto it = child_objects.begin(); it != child_objects.end(); ++it){
			const auto &child_object = *it;
			const auto map_object_type_id = child_object->get_map_object_type_id();
			if(map_object_type_id == MapObjectTypeIds::ID_CASTLE){
				continue;
			}
			child_object->set_coord(new_coord);
		}

		castle->pump_status();

		// 发邮件。
		try {
			const auto mail_uuid = MailUuid(Poseidon::Uuid::random());
			const auto language_id = LanguageId(); // neutral

			std::vector<std::pair<ChatMessageSlotId, std::string>> segments;
			segments.reserve(items_regained.size() + defenses_destroyed.size());
			for(auto it = items_regained.begin(); it != items_regained.end(); ++it){
				segments.emplace_back(ChatMessageSlotIds::ID_HUP_REGAINED_ITEM_ID,    boost::lexical_cast<std::string>(it->first));
				segments.emplace_back(ChatMessageSlotIds::ID_HUP_REGAINED_ITEM_COUNT, boost::lexical_cast<std::string>(it->second));
			}
			for(auto it = defenses_destroyed.begin(); it != defenses_destroyed.end(); ++it){
				const auto defense = boost::dynamic_pointer_cast<DefenseBuilding>(*it);
				if(!defense){
					continue;
				}
				const auto building_id = defense->get_map_object_type_id();
				const auto building_level = defense->get_level();
				segments.emplace_back(ChatMessageSlotIds::ID_HUP_DESTROYED_DEFENSE_ID,    boost::lexical_cast<std::string>(building_id));
				segments.emplace_back(ChatMessageSlotIds::ID_HUP_DESTROYED_DEFENSE_LEVEL, boost::lexical_cast<std::string>(building_level));
			}
			segments.emplace_back(ChatMessageSlotIds::ID_HUP_LAST_COORD_X, boost::lexical_cast<std::string>(last_coord.x()));
			segments.emplace_back(ChatMessageSlotIds::ID_HUP_LAST_COORD_Y, boost::lexical_cast<std::string>(last_coord.y()));
			segments.emplace_back(ChatMessageSlotIds::ID_HUP_CASTLE_NAME, castle->get_name());

			const auto utc_now = Poseidon::get_utc_time();

			const auto mail_data = boost::make_shared<MailData>(mail_uuid, language_id, utc_now,
				ChatMessageTypeIds::ID_CASTLE_HUNG_UP, AccountUuid(), std::string(), std::move(segments),
				boost::container::flat_map<ItemId, std::uint64_t>());
			MailBoxMap::insert_mail_data(mail_data);

			MailBox::MailInfo mail_info = { };
			mail_info.mail_uuid   = mail_uuid;
			mail_info.expiry_time = UINT64_MAX;
			mail_info.system      = true;
			mail_box->insert(std::move(mail_info));
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
		}
	};
	Poseidon::enqueue_async_job(really_hang_up);
} catch(std::exception &e){
	LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
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
}

std::pair<long, std::string> is_under_protection(const boost::shared_ptr<MapObject> &attacking_object,
	const boost::shared_ptr<MapObject> &attacked_object)
{
	PROFILE_ME;

	const auto attacking_account_uuid = attacking_object->get_owner_uuid();
	const auto attacked_account_uuid = attacked_object->get_owner_uuid();

	if(attacked_account_uuid && attacking_object->is_protectable()){
		// 处于保护状态下的防御建筑不能攻击其他玩家的部队。
		if(is_protection_in_effect(attacking_object)){
			return CbppResponse(Msg::ERR_BATTALION_UNDER_PROTECTION) <<attacking_object->get_map_object_uuid();
		}
	}
	if(attacking_account_uuid && attacked_object->is_protectable()){
		// 处于保护状态下的防御建筑不能遭到其他玩家的部队攻击。
		if(is_protection_in_effect(attacked_object)){
			return CbppResponse(Msg::ERR_BATTALION_UNDER_PROTECTION) <<attacked_object->get_map_object_uuid();
		}
		// 防御建筑不能遭到其他玩家处于保护状态下的部队的攻击。
		if(is_protection_in_effect(attacking_object)){
			return CbppResponse(Msg::ERR_BATTALION_UNDER_PROTECTION) <<attacking_object->get_map_object_uuid();
		}
	}
	return CbppResponse();
}
std::pair<long, std::string> is_under_protection(const boost::shared_ptr<MapObject> &attacking_object,
	const boost::shared_ptr<MapCell> &attacked_cell)
{
	PROFILE_ME;

	const auto attacking_account_uuid = attacking_object->get_owner_uuid();
	auto attacked_account_uuid = attacked_cell->get_occupier_owner_uuid();
	if(!attacked_account_uuid){
		attacked_account_uuid = attacked_cell->get_owner_uuid();
	}

	if(attacked_account_uuid && attacking_object->is_protectable()){
		// 处于保护状态下的领地不能攻击其他玩家的部队。
		if(is_protection_in_effect(attacking_object)){
			return CbppResponse(Msg::ERR_BATTALION_UNDER_PROTECTION) <<attacking_object->get_map_object_uuid();
		}
	}
	if(attacking_account_uuid && attacked_cell->is_protectable()){
		// 处于保护状态下的领地不能遭到其他玩家的部队攻击。
		if(is_protection_in_effect(attacked_cell)){
			return CbppResponse(Msg::ERR_MAP_CELL_UNDER_PROTECTION) <<attacked_cell->get_coord();
		}
		// 领地不能遭到其他玩家处于保护状态下的部队的攻击。
		if(is_protection_in_effect(attacking_object)){
			return CbppResponse(Msg::ERR_BATTALION_UNDER_PROTECTION) <<attacking_object->get_map_object_uuid();
		}
	}
	return CbppResponse();
}

}
