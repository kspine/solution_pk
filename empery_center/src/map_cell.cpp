#include "precompiled.hpp"
#include "map_cell.hpp"
#include "map_object.hpp"
#include "mysql/map_cell.hpp"
#include "singletons/world_map.hpp"
#include "player_session.hpp"
#include "cluster_session.hpp"
#include "msg/sc_map.hpp"
#include "msg/sk_map.hpp"
#include "transaction_element.hpp"
#include "reason_ids.hpp"
#include "castle.hpp"
#include "checked_arithmetic.hpp"
#include "data/map.hpp"
#include "singletons/account_map.hpp"
#include "data/global.hpp"
#include "attribute_ids.hpp"
#include "resource_ids.hpp"

namespace EmperyCenter {

MapCell::MapCell(Coord coord)
	: m_obj(
		[&]{
			auto obj = boost::make_shared<MySql::Center_MapCell>(coord.x(), coord.y(),
				Poseidon::Uuid(), false, 0, 0, 0, 0);
			obj->enable_auto_saving(); // obj->async_save(true);
			return obj;
		}())
{
}
MapCell::MapCell(boost::shared_ptr<MySql::Center_MapCell> obj,
	const std::vector<boost::shared_ptr<MySql::Center_MapCellAttribute>> &attributes)
	: m_obj(std::move(obj))
{
	for(auto it = attributes.begin(); it != attributes.end(); ++it){
		m_attributes.emplace(AttributeId((*it)->get_attribute_id()), *it);
	}
}
MapCell::~MapCell(){
}

void MapCell::pump_status(){
	PROFILE_ME;

	const auto coord = get_coord();
	const auto cluster_scope = WorldMap::get_cluster_scope(coord);
	const auto map_x = static_cast<unsigned>(coord.x() - cluster_scope.left());
	const auto map_y = static_cast<unsigned>(coord.y() - cluster_scope.bottom());
	LOG_EMPERY_CENTER_DEBUG("Updating map cell: coord = ", coord, ", cluster_scope = ", cluster_scope,
		", map_x = ", map_x, ", map_y = ", map_y);
	const auto cell_data = Data::MapCellBasic::require(map_x, map_y);
	const auto terrain_id = cell_data->terrain_id;

	const auto utc_now = Poseidon::get_utc_time();

	const bool acc_card_applied = m_obj->get_acceleration_card_applied();
	const auto ticket_item_id = get_ticket_item_id();
	const auto production_resource_id = get_production_resource_id();
	if(ticket_item_id && production_resource_id){
		const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(get_parent_object_uuid()));
		if(!castle){
			LOG_EMPERY_CENTER_DEBUG("No parent castle: coord = ", coord, ", parent_object_uuid = ", get_parent_object_uuid());
			DEBUG_THROW(Exception, sslit("No parent castle"));
		}

		const auto ticket_data     = Data::MapCellTicket::require(ticket_item_id);
		const auto production_data = Data::MapTerrain::require(terrain_id);
		double production_rate     = production_data->best_production_rate;
		double capacity            = production_data->best_capacity;

		if(production_resource_id != production_data->best_resource_id){
			const auto non_best_rate_modifier     = Data::Global::as_double(Data::Global::SLOT_NON_BEST_RESOURCE_PRODUCTION_RATE_MODIFIER);
			const auto non_best_capacity_modifier = Data::Global::as_double(Data::Global::SLOT_NON_BEST_RESOURCE_CAPACITY_MODIFIER);

			production_rate *= non_best_rate_modifier;
			capacity        *= non_best_capacity_modifier;
		}

		production_rate *= ticket_data->production_rate_modifier;
		capacity        *= ticket_data->capacity_modifier;

		if(acc_card_applied){
			const auto acc_card_rate_modifier     = Data::Global::as_double(Data::Global::SLOT_ACCELERATION_CARD_PRODUCTION_RATE_MODIFIER);
			const auto acc_card_capacity_modifier = Data::Global::as_double(Data::Global::SLOT_ACCELERATION_CARD_CAPACITY_MODIFIER);

			production_rate *= acc_card_rate_modifier;
			capacity        *= acc_card_capacity_modifier;
		}

		double turbo = 0.0;
		if(production_resource_id == ResourceIds::ID_GRAIN){
			turbo = castle->get_attribute(AttributeIds::ID_PRODUCTION_TURBO_GRAIN) / 1000.0;
		} else if(production_resource_id == ResourceIds::ID_WOOD){
			turbo = castle->get_attribute(AttributeIds::ID_PRODUCTION_TURBO_WOOD) / 1000.0;
		} else if(production_resource_id == ResourceIds::ID_STONE){
			turbo = castle->get_attribute(AttributeIds::ID_PRODUCTION_TURBO_STONE) / 1000.0;
		} else {
			LOG_EMPERY_CENTER_DEBUG("Unhandled production resource: production_resource_id = ", production_resource_id);
		}
		turbo += castle->get_attribute(AttributeIds::ID_PRODUCTION_TURBO_ALL) / 1000.0;
		production_rate *= (1 + turbo);

		if(production_rate < 0){
			production_rate = 0;
		}
		if(capacity < 0){
			capacity = 0;
		}
		LOG_EMPERY_CENTER_DEBUG("Checking map cell production: coord = ", get_coord(),
			", terrain_id = ", terrain_id, ", acc_card_applied = ", acc_card_applied,
			", ticket_item_id = ", ticket_item_id, ", production_resource_id = ", production_resource_id,
			", production_rate = ", production_rate, ", capacity = ", capacity);

		const auto old_last_production_time = m_obj->get_last_production_time();
		const auto old_resource_amount      = m_obj->get_resource_amount();

		const auto production_duration = saturated_sub(utc_now, old_last_production_time);
		const auto new_resource_amount = std::min(
			saturated_add(old_resource_amount,
				static_cast<std::uint64_t>(std::round(production_duration * production_rate / 60000.0))),
			static_cast<std::uint64_t>(std::round(capacity)));
		LOG_EMPERY_CENTER_DEBUG("Produced resource: coord = ", get_coord(),
			", terrain_id = ", terrain_id, ", new_resource_amount = ", new_resource_amount);

		m_obj->set_last_production_time (utc_now);
		m_obj->set_resource_amount      (new_resource_amount);
	}
}

Coord MapCell::get_coord() const {
	return Coord(m_obj->get_x(), m_obj->get_y());
}

MapObjectUuid MapCell::get_parent_object_uuid() const {
	return MapObjectUuid(m_obj->unlocked_get_parent_object_uuid());
}
AccountUuid MapCell::get_owner_uuid() const {
	PROFILE_ME;

	const auto parent_object_uuid = get_parent_object_uuid();
	if(!parent_object_uuid){
		return { };
	}
	const auto parent_object = WorldMap::get_map_object(parent_object_uuid);
	if(!parent_object){
		return { };
	}
	return parent_object->get_owner_uuid();
}

bool MapCell::is_acceleration_card_applied() const {
	return m_obj->get_acceleration_card_applied();
}
void MapCell::set_acceleration_card_applied(bool value){
	m_obj->set_acceleration_card_applied(value);
}

ItemId MapCell::get_ticket_item_id() const {
	return ItemId(m_obj->get_ticket_item_id());
}
ResourceId MapCell::get_production_resource_id() const {
	return ResourceId(m_obj->get_production_resource_id());
}
std::uint64_t MapCell::get_last_production_time() const {
	return m_obj->get_last_production_time();
}
std::uint64_t MapCell::get_resource_amount() const {
	return m_obj->get_resource_amount();
}

void MapCell::set_owner(MapObjectUuid parent_object_uuid, ResourceId production_resource_id, ItemId ticket_item_id){
	PROFILE_ME;

	pump_status();

	const auto now = Poseidon::get_utc_time();

	m_obj->set_parent_object_uuid     (parent_object_uuid.get());
	m_obj->set_ticket_item_id         (ticket_item_id.get());
	m_obj->set_production_resource_id (production_resource_id.get());
	m_obj->set_last_production_time   (now);
	m_obj->set_resource_amount        (0);

	WorldMap::update_map_cell(virtual_shared_from_this<MapCell>(), false);
}
void MapCell::set_ticket_item_id(ItemId ticket_item_id){
	PROFILE_ME;

	pump_status();

	m_obj->set_ticket_item_id         (ticket_item_id.get());

	WorldMap::update_map_cell(virtual_shared_from_this<MapCell>(), false);
}

std::uint64_t MapCell::harvest(std::uint64_t max_amount, bool saturated){
	PROFILE_ME;

	const auto coord = get_coord();
	const auto ticket_item_id = get_ticket_item_id();
	if(!ticket_item_id){
		LOG_EMPERY_CENTER_DEBUG("No ticket on map cell: coord = ", coord);
		return 0;
	}

	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(get_parent_object_uuid()));
	if(!castle){
		LOG_EMPERY_CENTER_DEBUG("No parent castle: coord = ", coord, ", parent_object_uuid = ", get_parent_object_uuid());
		return 0;
	}

	const auto resource_id = get_production_resource_id();
	if(!resource_id){
		LOG_EMPERY_CENTER_DEBUG("No production resource id: coord = ", coord);
		return 0;
	}
	auto amount_avail = get_resource_amount();
	if(amount_avail > max_amount){
		amount_avail = max_amount;
	}
	if(amount_avail == 0){
		LOG_EMPERY_CENTER_DEBUG("Zero amount specified: coord = ", coord);
		return 0;
	}

	auto amount_to_add = amount_avail;
	const auto max_amount_to_add = saturated_sub(castle->get_max_resource_amount(resource_id), castle->get_resource(resource_id).amount);
	if(amount_to_add > max_amount_to_add){
		amount_to_add = max_amount_to_add;
	}
	auto amount_to_remove = amount_avail;
	if(!saturated){
		amount_to_remove = amount_to_add;
	}
	LOG_EMPERY_CENTER_DEBUG("Harvesting resource: coord = ", coord,
		", castle_uuid = ", castle->get_map_object_uuid(), ", ticket_item_id = ", ticket_item_id,
		", resource_id = ", resource_id, ", amount_to_add = ", amount_to_add, ", amount_to_remove = ", amount_to_remove);

	std::vector<ResourceTransactionElement> transaction;
	transaction.emplace_back(ResourceTransactionElement::OP_ADD, resource_id, amount_to_add,
		ReasonIds::ID_HARVEST_MAP_CELL, coord.x(), coord.y(), ticket_item_id.get());
	castle->commit_resource_transaction(transaction,
		[&]{ m_obj->set_resource_amount(checked_sub(m_obj->get_resource_amount(), amount_to_remove)); });

	return amount_to_remove;
}

std::int64_t MapCell::get_attribute(AttributeId attribute_id) const {
	PROFILE_ME;

	const auto it = m_attributes.find(attribute_id);
	if(it == m_attributes.end()){
		return 0;
	}
	return it->second->get_value();
}
void MapCell::get_attributes(boost::container::flat_map<AttributeId, std::int64_t> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_attributes.size());
	for(auto it = m_attributes.begin(); it != m_attributes.end(); ++it){
		ret[it->first] = it->second->get_value();
	}
}
void MapCell::set_attributes(const boost::container::flat_map<AttributeId, std::int64_t> &modifiers){
	PROFILE_ME;

	for(auto it = modifiers.begin(); it != modifiers.end(); ++it){
		const auto obj_it = m_attributes.find(it->first);
		if(obj_it == m_attributes.end()){
			auto obj = boost::make_shared<MySql::Center_MapCellAttribute>(m_obj->get_x(), m_obj->get_y(),
				it->first.get(), 0);
			obj->enable_auto_saving(); // obj->async_save(true);
			m_attributes.emplace(it->first, std::move(obj));
		}
	}
	for(auto it = modifiers.begin(); it != modifiers.end(); ++it){
		const auto &obj = m_attributes.at(it->first);
		obj->set_value(it->second);
	}

	WorldMap::update_map_cell(virtual_shared_from_this<MapCell>(), false);
}

bool MapCell::is_virtually_removed() const {
	return !get_parent_object_uuid();
}
void MapCell::synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	if(is_virtually_removed()){
		Msg::SC_MapCellRemoved msg;
		msg.x                         = get_coord().x();
		msg.y                         = get_coord().y();
		session->send(msg);
	} else {
		const auto owner_uuid = get_owner_uuid();
		if(owner_uuid){
			AccountMap::cached_synchronize_account_with_player(owner_uuid, session);
		}

		Msg::SC_MapCellInfo msg;
		msg.x                         = get_coord().x();
		msg.y                         = get_coord().y();
		msg.parent_object_uuid        = get_parent_object_uuid().str();
		msg.owner_uuid                = owner_uuid.str();
		msg.acceleration_card_applied = is_acceleration_card_applied();
		msg.ticket_item_id            = get_ticket_item_id().get();
		msg.production_resource_id    = get_production_resource_id().get();
		msg.resource_amount           = get_resource_amount();
		msg.attributes.reserve(m_attributes.size());
		for(auto it = m_attributes.begin(); it != m_attributes.end(); ++it){
			auto &attribute = *msg.attributes.emplace(msg.attributes.end());
			attribute.attribute_id = it->first.get();
			attribute.value        = it->second->get_value();
		}
		session->send(msg);
	}
}
void MapCell::synchronize_with_cluster(const boost::shared_ptr<ClusterSession> &cluster) const {
	PROFILE_ME;

	Msg::SK_MapAddMapCell msg;
	msg.x                  = get_coord().x();
	msg.y                  = get_coord().y();
	msg.parent_object_uuid = get_parent_object_uuid().str();
	msg.owner_uuid         = get_owner_uuid().str();
	msg.attributes.reserve(m_attributes.size());
	for(auto it = m_attributes.begin(); it != m_attributes.end(); ++it){
		auto &attribute = *msg.attributes.emplace(msg.attributes.end());
		attribute.attribute_id = it->first.get();
		attribute.value        = it->second->get_value();
	}
	cluster->send(msg);
}

}
