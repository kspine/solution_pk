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
#include "account.hpp"
#include "data/vip.hpp"
#include "terrain_ids.hpp"
#include "buff_ids.hpp"
#include <poseidon/async_job.hpp>
#include "item_box.hpp"
#include "singletons/item_box_map.hpp"
#include "transaction_element.hpp"
#include "item_ids.hpp"
#include "data/castle.hpp"
#include "map_utilities.hpp"

namespace EmperyCenter {

namespace {
	void fill_buff_info(MapCell::BuffInfo &info, const boost::shared_ptr<MySql::Center_MapCellBuff> &obj){
		PROFILE_ME;

		info.buff_id    = BuffId(obj->get_buff_id());
		info.duration   = obj->get_duration();
		info.time_begin = obj->get_time_begin();
		info.time_end   = obj->get_time_end();
	}
}

MapCell::MapCell(Coord coord)
	: m_obj(
		[&]{
			auto obj = boost::make_shared<MySql::Center_MapCell>(coord.x(), coord.y(),
				Poseidon::Uuid(), false, 0, 0, 0, 0, Poseidon::Uuid(), 0);
			obj->async_save(true);
			return obj;
		}())
{
}
MapCell::MapCell(boost::shared_ptr<MySql::Center_MapCell> obj,
	const std::vector<boost::shared_ptr<MySql::Center_MapCellAttribute>> &attributes,
	const std::vector<boost::shared_ptr<MySql::Center_MapCellBuff>> &buffs)
	: m_obj(std::move(obj))
{
	for(auto it = attributes.begin(); it != attributes.end(); ++it){
		m_attributes.emplace(AttributeId((*it)->get_attribute_id()), *it);
	}
	for(auto it = buffs.begin(); it != buffs.end(); ++it){
		m_buffs.emplace(BuffId((*it)->get_buff_id()), *it);
	}
}
MapCell::~MapCell(){
}

bool MapCell::should_auto_update() const {
	PROFILE_ME;

	if(get_parent_object_uuid()){
		return true;
	}
	return false;
}
void MapCell::pump_status(){
	PROFILE_ME;

	pump_production();
	self_heal();
	check_occupation();
}
void MapCell::recalculate_attributes(bool recursive){
	PROFILE_ME;

	(void)recursive;

	// 无事可做。
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

	WorldMap::update_map_cell(virtual_shared_from_this<MapCell>(), false);
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

void MapCell::pump_production(){
	PROFILE_ME;

	const auto ticket_item_id = get_ticket_item_id();
	const auto last_production_time = get_last_production_time();
	if(!ticket_item_id && (last_production_time == 0)){
		return;
	}

	const auto coord = get_coord();
	const auto cluster_scope = WorldMap::get_cluster_scope(coord);
	const auto map_x = static_cast<unsigned>(coord.x() - cluster_scope.left());
	const auto map_y = static_cast<unsigned>(coord.y() - cluster_scope.bottom());
	LOG_EMPERY_CENTER_TRACE("Updating map cell: coord = ", coord, ", cluster_scope = ", cluster_scope,
		", map_x = ", map_x, ", map_y = ", map_y);
	const auto cell_data = Data::MapCellBasic::require(map_x, map_y);
	const auto terrain_id = cell_data->terrain_id;

	const auto utc_now = Poseidon::get_utc_time();

	double production_rate = 0;
	double capacity = 0;

	const auto production_resource_id = get_production_resource_id();
	const bool acc_card_applied = m_obj->get_acceleration_card_applied();

	if(ticket_item_id && production_resource_id){
		const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(get_parent_object_uuid()));
		if(!castle){
			LOG_EMPERY_CENTER_DEBUG("No parent castle: coord = ", coord, ", parent_object_uuid = ", get_parent_object_uuid());
			DEBUG_THROW(Exception, sslit("No parent castle"));
		}
		const auto account = AccountMap::require(castle->get_owner_uuid());

		const auto ticket_data     = Data::MapCellTicket::require(ticket_item_id);
		const auto production_data = Data::MapTerrain::require(terrain_id);

		production_rate     = production_data->best_production_rate;
		capacity            = production_data->best_capacity;

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

		double tech_turbo;
		if(production_resource_id == ResourceIds::ID_GRAIN){
			tech_turbo = castle->get_attribute(AttributeIds::ID_PRODUCTION_TURBO_GRAIN) / 1000.0;
		} else if(production_resource_id == ResourceIds::ID_WOOD){
			tech_turbo = castle->get_attribute(AttributeIds::ID_PRODUCTION_TURBO_WOOD) / 1000.0;
		} else if(production_resource_id == ResourceIds::ID_STONE){
			tech_turbo = castle->get_attribute(AttributeIds::ID_PRODUCTION_TURBO_STONE) / 1000.0;
		} else {
			LOG_EMPERY_CENTER_TRACE("Unhandled production resource: production_resource_id = ", production_resource_id);
			tech_turbo = 0;
		}
		switch(terrain_id.get()){
		case TerrainIds::ID_DESERT.get():
		case TerrainIds::ID_ROTTEN_WOOD.get():
		case TerrainIds::ID_GRAVEL.get():
		case TerrainIds::ID_OASIS.get():
			tech_turbo += castle->get_attribute(AttributeIds::ID_DESERT_DEVELOPMENT) / 1000.0;
			break;
		case TerrainIds::ID_WASTELAND.get():
		case TerrainIds::ID_DEADWOOD.get():
		case TerrainIds::ID_STONES.get():
			tech_turbo += castle->get_attribute(AttributeIds::ID_WASTELAND_DEVELOPMENT) / 1000.0;
			break;
		case TerrainIds::ID_PRAIRIE.get():
		case TerrainIds::ID_FOREST.get():
		case TerrainIds::ID_HILL.get():
		case TerrainIds::ID_SOIL.get():
			tech_turbo += castle->get_attribute(AttributeIds::ID_PRAIRIE_DEVELOPMENT) / 1000.0;
			break;
		}
		tech_turbo += castle->get_attribute(AttributeIds::ID_PRODUCTION_TURBO_ALL) / 1000.0;
		production_rate *= (1 + tech_turbo);

		const auto vip_data = Data::Vip::require(account->get_promotion_level());
		production_rate *= (1 + vip_data->production_turbo);

		capacity = std::round(capacity);

		if(production_rate < 0){
			production_rate = 0;
		}
		if(capacity < 0){
			capacity = 0;
		}
		LOG_EMPERY_CENTER_TRACE("Checking map cell production: coord = ", get_coord(),
			", terrain_id = ", terrain_id, ", acc_card_applied = ", acc_card_applied,
			", ticket_item_id = ", ticket_item_id, ", production_resource_id = ", production_resource_id,
			", production_rate = ", production_rate, ", capacity = ", capacity);

		const auto old_last_production_time = m_obj->get_last_production_time();
		const auto old_resource_amount      = m_obj->get_resource_amount();

		const auto production_duration = saturated_sub(utc_now, old_last_production_time);
		const auto amount_produced = production_duration * production_rate / 60000.0 + m_production_remainder;
		const auto rounded_amount_produced = static_cast<std::uint64_t>(amount_produced);
		const auto new_resource_amount = std::min<std::uint64_t>(saturated_add(old_resource_amount, rounded_amount_produced), capacity);
		if(new_resource_amount > old_resource_amount){
			m_obj->set_resource_amount(new_resource_amount);
		}
		m_production_remainder = amount_produced - rounded_amount_produced;
	} else {
		m_obj->set_resource_amount(0);
		m_production_remainder = 0;
	}
	m_obj->set_last_production_time(utc_now);

	m_production_rate = production_rate;
	m_capacity        = capacity;
}

void MapCell::set_parent_object(MapObjectUuid parent_object_uuid, ResourceId production_resource_id, ItemId ticket_item_id){
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

std::uint64_t MapCell::harvest(const boost::shared_ptr<Castle> &castle, double amount_to_harvest, bool saturated){
	PROFILE_ME;

	const auto coord = get_coord();
	const auto ticket_item_id = get_ticket_item_id();
	if(!ticket_item_id){
		LOG_EMPERY_CENTER_DEBUG("No ticket on map cell: coord = ", coord);
		return 0;
	}

	const auto resource_id = get_production_resource_id();
	if(!resource_id){
		LOG_EMPERY_CENTER_DEBUG("No production resource id: coord = ", coord);
		return 0;
	}
	const auto amount_remaining = get_resource_amount();
	if(amount_remaining == 0){
		LOG_EMPERY_CENTER_DEBUG("No resource available: coord = ", coord);
		return 0;
	}

	amount_to_harvest += m_harvest_remainder;

	const auto rounded_amount_to_harvest = static_cast<std::uint64_t>(amount_to_harvest);
	const auto rounded_amount_removable = std::min(rounded_amount_to_harvest, amount_remaining);
	const auto capacity_remaining = saturated_sub(castle->get_warehouse_capacity(resource_id),
	                                              castle->get_resource(resource_id).amount);
	const auto amount_added = std::min(rounded_amount_removable, capacity_remaining);
	{
		std::vector<ResourceTransactionElement> transaction;
		transaction.emplace_back(ResourceTransactionElement::OP_ADD, resource_id, amount_added,
			ReasonIds::ID_HARVEST_MAP_CELL, coord.x(), coord.y(), ticket_item_id.get());
		castle->commit_resource_transaction(transaction);
	}
	const auto amount_removed = saturated ? rounded_amount_removable : amount_added;
	m_obj->set_resource_amount(saturated_sub(amount_remaining, amount_removed));

	m_harvest_remainder = amount_to_harvest - rounded_amount_to_harvest;

	WorldMap::update_map_cell(virtual_shared_from_this<MapCell>(), false);

	return amount_removed;
}
std::uint64_t MapCell::harvest(const boost::shared_ptr<MapObject> &harvester, double amount_to_harvest, bool saturated){
	PROFILE_ME;

	const auto coord = get_coord();
	const auto ticket_item_id = get_ticket_item_id();
	if(!ticket_item_id){
		LOG_EMPERY_CENTER_DEBUG("No ticket on map cell: coord = ", coord);
		return 0;
	}

	const auto resource_id = get_production_resource_id();
	if(!resource_id){
		LOG_EMPERY_CENTER_DEBUG("No production resource id: coord = ", coord);
		return 0;
	}
	const auto amount_remaining = get_resource_amount();
	if(amount_remaining == 0){
		LOG_EMPERY_CENTER_DEBUG("No resource available: coord = ", coord);
		return 0;
	}

	amount_to_harvest += m_harvest_remainder;

	const auto rounded_amount_to_harvest = static_cast<std::uint64_t>(amount_to_harvest);
	const auto rounded_amount_removable = std::min(rounded_amount_to_harvest, amount_remaining);
	const auto amount_added = harvester->load_resource(resource_id, rounded_amount_removable, false);
	const auto amount_removed = saturated ? rounded_amount_removable : amount_added;
	m_obj->set_resource_amount(saturated_sub(amount_remaining, amount_removed));

	m_harvest_remainder = amount_to_harvest - rounded_amount_to_harvest;

	WorldMap::update_map_cell(virtual_shared_from_this<MapCell>(), false);

	return amount_removed;
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
			obj->async_save(true);
			m_attributes.emplace(it->first, std::move(obj));
		}
	}
	for(auto it = modifiers.begin(); it != modifiers.end(); ++it){
		const auto &obj = m_attributes.at(it->first);
		obj->set_value(it->second);
	}

	WorldMap::update_map_cell(virtual_shared_from_this<MapCell>(), false);
}

MapCell::BuffInfo MapCell::get_buff(BuffId buff_id) const {
	PROFILE_ME;

	BuffInfo info = { };
	info.buff_id = buff_id;
	const auto it = m_buffs.find(buff_id);
	if(it == m_buffs.end()){
		return info;
	}
	if(it->second->get_time_end() == 0){
		return info;
	}
	fill_buff_info(info, it->second);
	return info;
}
bool MapCell::is_buff_in_effect(BuffId buff_id) const {
	PROFILE_ME;

	const auto it = m_buffs.find(buff_id);
	if(it == m_buffs.end()){
		return false;
	}
	const auto time_end = it->second->get_time_end();
	if(time_end == 0){
		return false;
	}
	const auto utc_now = Poseidon::get_utc_time();
	return utc_now < time_end;
}
void MapCell::get_buffs(std::vector<MapCell::BuffInfo> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_buffs.size());
	for(auto it = m_buffs.begin(); it != m_buffs.end(); ++it){
		if(it->second->get_time_end() == 0){
			continue;
		}
		BuffInfo info;
		fill_buff_info(info, it->second);
		ret.emplace_back(std::move(info));
	}
}
void MapCell::set_buff(BuffId buff_id, std::uint64_t duration){
	PROFILE_ME;

	const auto utc_now = Poseidon::get_utc_time();
	set_buff(buff_id, utc_now, duration);
}
void MapCell::set_buff(BuffId buff_id, std::uint64_t time_begin, std::uint64_t duration){
	PROFILE_ME;

	auto it = m_buffs.find(buff_id);
	if(it == m_buffs.end()){
		auto obj = boost::make_shared<MySql::Center_MapCellBuff>(m_obj->get_x(), m_obj->get_y(),
			buff_id.get(), 0, 0, 0);
		obj->async_save(true);
		it = m_buffs.emplace(buff_id, std::move(obj)).first;
	}
	const auto &obj = it->second;
	obj->set_duration(duration);
	obj->set_time_begin(time_begin);
	obj->set_time_end(saturated_add(time_begin, duration));

	WorldMap::update_map_cell(virtual_shared_from_this<MapCell>(), false);
}
void MapCell::accumulate_buff(BuffId buff_id, std::uint64_t delta_duration){
	PROFILE_ME;

	auto it = m_buffs.find(buff_id);
	if(it == m_buffs.end()){
		auto obj = boost::make_shared<MySql::Center_MapCellBuff>(m_obj->get_x(), m_obj->get_y(),
			buff_id.get(), 0, 0, 0);
		obj->async_save(true);
		it = m_buffs.emplace(buff_id, std::move(obj)).first;
	}
	const auto &obj = it->second;
	const auto utc_now = Poseidon::get_utc_time();
	const auto old_duration = obj->get_duration(), old_time_begin = obj->get_time_begin(), old_time_end = obj->get_time_end();
	std::uint64_t new_duration, new_time_begin;
	if(utc_now < old_time_end){
		new_duration = saturated_add(old_duration, delta_duration);
		new_time_begin = old_time_begin;
	} else {
		new_duration = delta_duration;
		new_time_begin = utc_now;
	}
	obj->set_duration(new_duration);
	obj->set_time_begin(new_time_begin);
	obj->set_time_end(saturated_add(new_time_begin, new_duration));

	WorldMap::update_map_cell(virtual_shared_from_this<MapCell>(), false);
}
void MapCell::clear_buff(BuffId buff_id) noexcept {
	PROFILE_ME;

	const auto it = m_buffs.find(buff_id);
	if(it == m_buffs.end()){
		return;
	}
	const auto obj = std::move(it->second);
	m_buffs.erase(it);

	obj->set_duration(0);
	obj->set_time_begin(0);
	obj->set_time_end(0);

	WorldMap::update_map_cell(virtual_shared_from_this<MapCell>(), false);
}

void MapCell::self_heal(){
	PROFILE_ME;

	const auto ticket_item_id = get_ticket_item_id();
	if(!ticket_item_id){
		return;
	}
	const auto ticket_data = Data::MapCellTicket::require(ticket_item_id);
	const auto self_healing_rate = ticket_data->self_healing_rate;
	if(self_healing_rate <= 0){
		return;
	}
	const auto max_soldier_count = ticket_data->soldiers_max;
	if(max_soldier_count <= 0){
		return;
	}
	const auto soldiers_healed_perminute = std::ceil(self_healing_rate * max_soldier_count - 0.001);

	LOG_EMPERY_CENTER_TRACE("Self heal: coord = ", get_coord(), ", ticket_item_id = ", ticket_item_id,
		", self_healing_rate = ", self_healing_rate, ", max_soldier_count = ", max_soldier_count);

	const auto old_self_healed_time = m_obj->get_last_self_healed_time();
	const auto old_soldier_count = static_cast<std::uint64_t>(get_attribute(AttributeIds::ID_SOLDIER_COUNT));

	const auto utc_now = Poseidon::get_utc_time();

	const auto self_healing_duration = saturated_sub(utc_now, old_self_healed_time);
	const auto amount_healed = self_healing_duration * soldiers_healed_perminute / 60000.0 + m_self_healing_remainder;
	const auto rounded_amount_healed = static_cast<std::uint64_t>(amount_healed);
	const auto new_soldier_count = std::min<std::uint64_t>(saturated_add(old_soldier_count, rounded_amount_healed), max_soldier_count);
	if(new_soldier_count > old_soldier_count){
		boost::container::flat_map<AttributeId, std::int64_t> modifiers;
		modifiers.reserve(16);
		modifiers[AttributeIds::ID_SOLDIER_COUNT_MAX] = static_cast<std::int64_t>(max_soldier_count);
		modifiers[AttributeIds::ID_SOLDIER_COUNT]     = static_cast<std::int64_t>(new_soldier_count);
		set_attributes(std::move(modifiers));
	}
	m_self_healing_remainder = amount_healed - rounded_amount_healed;
	m_obj->set_last_self_healed_time(utc_now);
}

MapObjectUuid MapCell::get_occupier_object_uuid() const {
	PROFILE_ME;

	return MapObjectUuid(m_obj->unlocked_get_occupier_object_uuid());
}
AccountUuid MapCell::get_occupier_owner_uuid() const {
	PROFILE_ME;

	const auto occupier_object_uuid = get_occupier_object_uuid();
	if(!occupier_object_uuid){
		return { };
	}
	const auto occupier_object = WorldMap::get_map_object(occupier_object_uuid);
	if(!occupier_object){
		return { };
	}
	return occupier_object->get_owner_uuid();
}
void MapCell::set_occupier_object_uuid(MapObjectUuid occupier_object_uuid){
	PROFILE_ME;

	m_obj->set_occupier_object_uuid(occupier_object_uuid.get());

	WorldMap::update_map_cell(virtual_shared_from_this<MapCell>(), false);
}

void MapCell::check_occupation(){
	PROFILE_ME;

	const auto return_map_cell = [&](const boost::shared_ptr<MapCell> & /* this */){
		PROFILE_ME;

		const auto occupier_object_uuid = get_occupier_object_uuid();
		if(!occupier_object_uuid){
			return;
		}

		const auto ticket_item_id = get_ticket_item_id();
		if(!ticket_item_id){
			return;
		}
		const auto parent_object_uuid = get_parent_object_uuid();
		if(!parent_object_uuid){
			return;
		}
		const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(parent_object_uuid));
		if(!castle){
			return;
		}

		const auto item_box = ItemBoxMap::require(castle->get_owner_uuid());

		const auto protection_minutes = Data::Global::as_unsigned(Data::Global::SLOT_MAP_CELL_PROTECTION_DURATION);
		const auto protection_duration = checked_mul<std::uint64_t>(protection_minutes, 60000);

		std::vector<ItemTransactionElement> transaction;
		bool ticket_reclaimed = false;
		const auto coord = get_coord();
		const auto castle_level = castle->get_level();
		const auto updrade_data = Data::CastleUpgradePrimary::require(castle_level);
		const auto distance = get_distance_of_coords(coord, castle->get_coord());
		if(distance > updrade_data->max_map_cell_distance){
			transaction.emplace_back(ItemTransactionElement::OP_ADD, ticket_item_id, 1,
				ReasonIds::ID_OCCUPATION_END_RELOCATED, coord.x(), coord.y(), 0);
			if(is_acceleration_card_applied()){
				transaction.emplace_back(ItemTransactionElement::OP_ADD, ItemIds::ID_ACCELERATION_CARD, 1,
					ReasonIds::ID_OCCUPATION_END_RELOCATED, coord.x(), coord.y(), 0);
			}
			ticket_reclaimed = true;
		}
		item_box->commit_transaction(transaction, false,
			[&]{
				set_buff(BuffIds::ID_MAP_CELL_OCCUPATION_PROTECTION, protection_duration);
				clear_buff(BuffIds::ID_MAP_CELL_OCCUPATION);
				set_occupier_object_uuid({ });

				if(ticket_reclaimed){
					set_parent_object({ }, { }, { });
					set_acceleration_card_applied(false);
				}
			});
	};

	const auto occupier_object_uuid = get_occupier_object_uuid();
	if(!occupier_object_uuid){
		return;
	}
	if(!is_buff_in_effect(BuffIds::ID_MAP_CELL_OCCUPATION)){
		Poseidon::enqueue_async_job(
			std::bind(return_map_cell, virtual_shared_from_this<MapCell>()));
	}
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
		const auto utc_now = Poseidon::get_utc_time();

		const auto parent_object_uuid = get_parent_object_uuid();
		boost::shared_ptr<MapObject> parent_object;
		if(parent_object_uuid){
			parent_object = WorldMap::get_map_object(parent_object_uuid);
		}
		const auto owner_uuid = get_owner_uuid();
		if(owner_uuid){
			AccountMap::cached_synchronize_account_with_player(owner_uuid, session);
		}

		const auto occupier_object_uuid = get_occupier_object_uuid();
		boost::shared_ptr<MapObject> occupier_object;
		if(occupier_object_uuid){
			occupier_object = WorldMap::get_map_object(occupier_object_uuid);
		}
		const auto occupier_owner_uuid = get_occupier_owner_uuid();
		if(occupier_owner_uuid){
			AccountMap::cached_synchronize_account_with_player(occupier_owner_uuid, session);
		}

		Msg::SC_MapCellInfo msg;
		msg.x                         = get_coord().x();
		msg.y                         = get_coord().y();
		msg.parent_object_uuid        = parent_object_uuid.str();
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
		msg.production_rate           = std::round(get_production_rate() * 1000);
		msg.capacity                  = get_capacity();
		if(parent_object){
			msg.parent_object_x = parent_object->get_coord().x();
			msg.parent_object_y = parent_object->get_coord().y();
		}
		msg.buffs.reserve(m_buffs.size());
		for(auto it = m_buffs.begin(); it != m_buffs.end(); ++it){
			auto &buff = *msg.buffs.emplace(msg.buffs.end());
			buff.buff_id        = it->first.get();
			buff.duration       = it->second->get_duration();
			buff.time_remaining = saturated_sub(it->second->get_time_end(), utc_now);
		}
		msg.occupier_object_uuid      = occupier_object_uuid.str();
		msg.occupier_owner_uuid       = occupier_owner_uuid.str();
		if(occupier_object){
			msg.occupier_x            = occupier_object->get_coord().x();
			msg.occupier_y            = occupier_object->get_coord().y();
		}
		session->send(msg);
	}
}
void MapCell::synchronize_with_cluster(const boost::shared_ptr<ClusterSession> &cluster) const {
	PROFILE_ME;

	Msg::SK_MapAddMapCell msg;
	msg.x                         = get_coord().x();
	msg.y                         = get_coord().y();
	msg.parent_object_uuid        = get_parent_object_uuid().str();
	msg.owner_uuid                = get_owner_uuid().str();
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
	msg.buffs.reserve(m_buffs.size());
	for(auto it = m_buffs.begin(); it != m_buffs.end(); ++it){
		auto &buff = *msg.buffs.emplace(msg.buffs.end());
		buff.buff_id    = it->first.get();
		buff.time_begin = it->second->get_time_begin();
		buff.time_end   = it->second->get_time_end();
	}
	msg.occupier_object_uuid      = get_occupier_object_uuid().str();
	msg.occupier_owner_uuid       = get_occupier_owner_uuid().str();
	cluster->send(msg);
}

}
