#include "precompiled.hpp"
#include "map_object.hpp"
#include "mysql/map_object.hpp"
#include "singletons/world_map.hpp"
#include "singletons/player_session_map.hpp"
#include "player_session.hpp"
#include "cluster_session.hpp"
#include "msg/sc_map.hpp"
#include "msg/sk_map.hpp"
#include "singletons/account_map.hpp"
#include "attribute_ids.hpp"
#include "data/map_object_type.hpp"
#include "data/map.hpp"
#include "data/castle.hpp"
#include "data/buff.hpp"
#include "castle.hpp"
#include "transaction_element.hpp"
#include "reason_ids.hpp"
#include "map_object_type_ids.hpp"

namespace EmperyCenter {

namespace {
	void fill_buff_info(MapObject::BuffInfo &info, const boost::shared_ptr<MySql::Center_MapObjectBuff> &obj){
		PROFILE_ME;

		info.buff_id    = BuffId(obj->get_buff_id());
		info.duration   = obj->get_duration();
		info.time_begin = obj->get_time_begin();
		info.time_end   = obj->get_time_end();
	}
}

const std::initializer_list<AttributeId> MapObject::COMBAT_ATTRIBUTES = {
	AttributeIds::ID_ATTACK_BONUS,
	AttributeIds::ID_DEFENSE_BONUS,
	AttributeIds::ID_DODGING_RATIO_BONUS,
	AttributeIds::ID_CRITICAL_DAMAGE_RATIO_BONUS,
	AttributeIds::ID_CRITICAL_DAMAGE_MULTIPLIER_BONUS,
	AttributeIds::ID_ATTACK_RANGE_BONUS,
	AttributeIds::ID_SIGHT_RANGE_BONUS,
	AttributeIds::ID_RATE_OF_FIRE_BONUS,
	AttributeIds::ID_SPEED_BONUS,
	AttributeIds::ID_ATTACK_ADD,
	AttributeIds::ID_DEFENSE_ADD,
	AttributeIds::ID_DODGING_RATIO_ADD,
	AttributeIds::ID_CRITICAL_DAMAGE_RATIO_ADD,
	AttributeIds::ID_CRITICAL_DAMAGE_MULTIPLIER_ADD,
	AttributeIds::ID_ATTACK_RANGE_ADD,
	AttributeIds::ID_SIGHT_RANGE_ADD,
	AttributeIds::ID_RATE_OF_FIRE_ADD,
	AttributeIds::ID_SPEED_ADD,
};

MapObject::MapObject(MapObjectUuid map_object_uuid, MapObjectTypeId map_object_type_id, AccountUuid owner_uuid,
	MapObjectUuid parent_object_uuid, std::string name, Coord coord, std::uint64_t created_time, bool garrisoned)
	: m_obj(
		[&]{
			auto obj = boost::make_shared<MySql::Center_MapObject>(map_object_uuid.get(), map_object_type_id.get(), owner_uuid.get(),
				parent_object_uuid.get(), std::move(name), coord.x(), coord.y(), created_time, false, garrisoned);
			obj->async_save(true, true);
			return obj;
		}())
{
}
MapObject::MapObject(boost::shared_ptr<MySql::Center_MapObject> obj,
	const std::vector<boost::shared_ptr<MySql::Center_MapObjectAttribute>> &attributes,
	const std::vector<boost::shared_ptr<MySql::Center_MapObjectBuff>> &buffs)
	: m_obj(std::move(obj))
{
	for(auto it = attributes.begin(); it != attributes.end(); ++it){
		m_attributes.emplace(AttributeId((*it)->get_attribute_id()), *it);
	}
	for(auto it = buffs.begin(); it != buffs.end(); ++it){
		m_buffs.emplace(BuffId((*it)->get_buff_id()), *it);
	}
}
MapObject::~MapObject(){
}

void MapObject::synchronize_with_player_additional(const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	(void)session;
}

bool MapObject::should_auto_update() const {
	PROFILE_ME;

	if(!is_garrisoned()){
		return true;
	}
	return false;
}
void MapObject::pump_status(){
	PROFILE_ME;

	bool dirty = false;

	boost::shared_ptr<MapObject> parent_object;
	const auto parent_object_uuid = get_parent_object_uuid();
	if(parent_object_uuid){
		parent_object = WorldMap::get_map_object(parent_object_uuid);
	}
	if(parent_object && (m_last_updated_time < parent_object->m_last_updated_time)){
		++dirty;
	}
	if(dirty){
		recalculate_attributes(false);
	}
}
void MapObject::recalculate_attributes(bool recursive){
	PROFILE_ME;

	(void)recursive;

	const auto utc_now = Poseidon::get_utc_time();

	boost::container::flat_map<AttributeId, std::int64_t> modifiers;
	modifiers.reserve(64);
	for(auto it = COMBAT_ATTRIBUTES.begin(); it != COMBAT_ATTRIBUTES.end(); ++it){
		const auto attribute_id = *it;
		modifiers.emplace_hint(modifiers.end(), attribute_id, 0);
	}

	const auto map_object_type_id = get_map_object_type_id();
	boost::shared_ptr<Castle> parent_castle, tech_castle;
	if(map_object_type_id == MapObjectTypeIds::ID_CASTLE){
		// parent_castle = { };
		tech_castle = virtual_shared_from_this<Castle>();
	} else {
		const auto parent_castle_uuid = get_parent_object_uuid();
		if(parent_castle_uuid){
			parent_castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(parent_castle_uuid));
		}
		tech_castle = parent_castle;
	}
	if(tech_castle){
		LOG_EMPERY_CENTER_TRACE("Updating attributes from castle: map_object_uuid = ", get_map_object_uuid(),
			", tech_castle_uuid = ", tech_castle->get_map_object_uuid());

		std::vector<boost::shared_ptr<const Data::MapObjectTypeAttributeBonus>> attribute_bonus_applicable_data;
		Data::MapObjectTypeAttributeBonus::get_applicable(attribute_bonus_applicable_data,
			Data::MapObjectTypeAttributeBonus::AKT_ALL, 0);
		const auto defense = boost::dynamic_pointer_cast<Castle>(shared_from_this());
		if(defense){
			const auto defense_data = Data::MapDefenseBuildingAbstract::get(map_object_type_id, defense->get_level());
			if(defense_data){
				const auto combat_data = Data::MapDefenseCombat::require(defense_data->defense_combat_id);
				Data::MapObjectTypeAttributeBonus::get_applicable(attribute_bonus_applicable_data,
					Data::MapObjectTypeAttributeBonus::AKT_WEAPON_ID, combat_data->map_object_weapon_id.get());
			}
		} else {
			const auto map_object_data = Data::MapObjectTypeAbstract::get(map_object_type_id);
			if(map_object_data){
				Data::MapObjectTypeAttributeBonus::get_applicable(attribute_bonus_applicable_data,
					Data::MapObjectTypeAttributeBonus::AKT_CHASSIS_ID, map_object_data->map_object_chassis_id.get());
				Data::MapObjectTypeAttributeBonus::get_applicable(attribute_bonus_applicable_data,
					Data::MapObjectTypeAttributeBonus::AKT_WEAPON_ID, map_object_data->map_object_weapon_id.get());
			}
		}
		Data::MapObjectTypeAttributeBonus::get_applicable(attribute_bonus_applicable_data,
			Data::MapObjectTypeAttributeBonus::AKT_MAP_OBJECT_TYPE_ID, map_object_type_id.get());
		for(auto it = attribute_bonus_applicable_data.begin(); it != attribute_bonus_applicable_data.end(); ++it){
			const auto &attribute_bonus_data = *it;
			const auto tech_attribute_id = attribute_bonus_data->tech_attribute_id;
			const auto bonus_attribute_id = attribute_bonus_data->bonus_attribute_id;
			const auto tech_attribute_value = tech_castle->get_attribute(tech_attribute_id);
			LOG_EMPERY_CENTER_TRACE("> Applying attribute bonus: tech_attribute_id = ", tech_attribute_id,
				", bonus_attribute_id = ", bonus_attribute_id, ", tech_attribute_value = ", tech_attribute_value);
			modifiers[bonus_attribute_id] += tech_attribute_value;
		}
	}
	if(parent_castle){
		for(auto it = parent_castle->m_buffs.begin(); it != parent_castle->m_buffs.end(); ++it){
			const auto buff_id = it->first;
			const auto time_end = it->second->get_time_end();
			if(utc_now >= time_end){
				continue;
			}
			const auto buff_data = Data::Buff::get(buff_id);
			if(!buff_data){
				continue;
			}
			const auto &attributes = buff_data->attributes;
			for(auto ait = attributes.begin(); ait != attributes.end(); ++ait){
				const auto attribute_id = ait->first;
				if(std::find(COMBAT_ATTRIBUTES.begin(), COMBAT_ATTRIBUTES.end(), attribute_id) == COMBAT_ATTRIBUTES.end()){
					continue;
				}
				auto &value = modifiers[ait->first];
				value += std::round(ait->second * 1000.0);
			}
		}
	}

	for(auto it = m_buffs.begin(); it != m_buffs.end(); ++it){
		const auto buff_id = it->first;
		const auto time_end = it->second->get_time_end();
		if(utc_now >= time_end){
			continue;
		}
		const auto buff_data = Data::Buff::get(buff_id);
		if(!buff_data){
			continue;
		}
		const auto &attributes = buff_data->attributes;
		for(auto ait = attributes.begin(); ait != attributes.end(); ++ait){
			auto &value = modifiers[ait->first];
			value += std::round(ait->second * 1000.0);
		}
	}

	set_attributes(std::move(modifiers));
	m_last_updated_time = utc_now;
}

bool MapObject::is_protectable() const {
	return false;
}

MapObjectUuid MapObject::get_map_object_uuid() const {
	return MapObjectUuid(m_obj->unlocked_get_map_object_uuid());
}
MapObjectTypeId MapObject::get_map_object_type_id() const {
	return MapObjectTypeId(m_obj->get_map_object_type_id());
}

AccountUuid MapObject::get_owner_uuid() const {
	return AccountUuid(m_obj->unlocked_get_owner_uuid());
}
MapObjectUuid MapObject::get_parent_object_uuid() const {
	return MapObjectUuid(m_obj->unlocked_get_parent_object_uuid());
}
Coord MapObject::get_coord() const {
	return Coord(m_obj->get_x(), m_obj->get_y());
}
void MapObject::set_coord(Coord coord) noexcept {
	PROFILE_ME;

	m_obj->set_x(coord.x());
	m_obj->set_y(coord.y());

	++m_stamp;

	WorldMap::update_map_object(virtual_shared_from_this<MapObject>(), false);
}
void MapObject::set_coord_no_synchronize(Coord coord) noexcept {
	PROFILE_ME;

	if(get_coord() == coord){
		return;
	}
	m_obj->set_x(coord.x());
	m_obj->set_y(coord.y());

	WorldMap::update_map_object(virtual_shared_from_this<MapObject>(), false);
}

std::uint64_t MapObject::get_created_time() const {
	return m_obj->get_created_time();
}

const std::string &MapObject::get_name() const {
	return m_obj->unlocked_get_name();
}
void MapObject::set_name(std::string name){
	PROFILE_ME;

	m_obj->set_name(std::move(name));

	WorldMap::update_map_object(virtual_shared_from_this<MapObject>(), false);
}

bool MapObject::has_been_deleted() const {
	return m_obj->get_deleted();
}
void MapObject::delete_from_game() noexcept {
	PROFILE_ME;

	if(has_been_deleted()){
		return;
	}
	m_obj->set_deleted(true);

	++m_stamp;

	WorldMap::update_map_object(virtual_shared_from_this<MapObject>(), false);
}

bool MapObject::is_garrisoned() const {
	return m_obj->get_garrisoned();
}
void MapObject::set_garrisoned(bool garrisoned){
	PROFILE_ME;

	m_obj->set_garrisoned(garrisoned);
	++m_stamp;

	WorldMap::update_map_object(virtual_shared_from_this<MapObject>(), false);
}

std::int64_t MapObject::get_attribute(AttributeId attribute_id) const {
	PROFILE_ME;

	const auto it = m_attributes.find(attribute_id);
	if(it == m_attributes.end()){
		return 0;
	}
	return it->second->get_value();
}
void MapObject::get_attributes(boost::container::flat_map<AttributeId, std::int64_t> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_attributes.size());
	for(auto it = m_attributes.begin(); it != m_attributes.end(); ++it){
		ret[it->first] = it->second->get_value();
	}
}
void MapObject::set_attributes(boost::container::flat_map<AttributeId, std::int64_t> modifiers){
	PROFILE_ME;

	for(auto it = modifiers.begin(); it != modifiers.end(); ++it){
		const auto obj_it = m_attributes.find(it->first);
		if(obj_it == m_attributes.end()){
			auto obj = boost::make_shared<MySql::Center_MapObjectAttribute>(m_obj->unlocked_get_map_object_uuid(),
				it->first.get(), 0);
			// obj->async_save(true);
			obj->enable_auto_saving();
			m_attributes.emplace(it->first, std::move(obj));
		}
	}

	bool dirty = false;
	for(auto it = modifiers.begin(); it != modifiers.end(); ++it){
		const auto &obj = m_attributes.at(it->first);
		const auto old_value = obj->get_value();
		if(old_value == it->second){
			continue;
		}
		obj->set_value(it->second);
		++dirty;
	}
	if(!dirty){
		return;
	}

	WorldMap::update_map_object(virtual_shared_from_this<MapObject>(), false);
}

MapObject::BuffInfo MapObject::get_buff(BuffId buff_id) const {
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
bool MapObject::is_buff_in_effect(BuffId buff_id) const {
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
void MapObject::get_buffs(std::vector<MapObject::BuffInfo> &ret) const {
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
void MapObject::set_buff(BuffId buff_id, std::uint64_t duration){
	PROFILE_ME;

	const auto utc_now = Poseidon::get_utc_time();
	set_buff(buff_id, utc_now, duration);
}
void MapObject::set_buff(BuffId buff_id, std::uint64_t time_begin, std::uint64_t duration){
	PROFILE_ME;

	auto it = m_buffs.find(buff_id);
	if(it == m_buffs.end()){
		auto obj = boost::make_shared<MySql::Center_MapObjectBuff>(m_obj->unlocked_get_map_object_uuid(),
			buff_id.get(), 0, 0, 0);
		obj->async_save(true);
		it = m_buffs.emplace(buff_id, std::move(obj)).first;
	}
	const auto &obj = it->second;
	obj->set_duration(duration);
	obj->set_time_begin(time_begin);
	obj->set_time_end(saturated_add(time_begin, duration));

	WorldMap::update_map_object(virtual_shared_from_this<MapObject>(), false);

	const auto buff_data = Data::Buff::get(buff_id);
	if(buff_data){
		try {
			recalculate_attributes(true);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
		}
	}
}
void MapObject::accumulate_buff(BuffId buff_id, std::uint64_t delta_duration){
	PROFILE_ME;

	auto it = m_buffs.find(buff_id);
	if(it == m_buffs.end()){
		auto obj = boost::make_shared<MySql::Center_MapObjectBuff>(m_obj->unlocked_get_map_object_uuid(),
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

	WorldMap::update_map_object(virtual_shared_from_this<MapObject>(), false);

	const auto buff_data = Data::Buff::get(buff_id);
	if(buff_data){
		try {
			recalculate_attributes(true);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
		}
	}
}
void MapObject::clear_buff(BuffId buff_id) noexcept {
	PROFILE_ME;

	const auto it = m_buffs.find(buff_id);
	if(it == m_buffs.end()){
		return;
	}
	const auto &obj = it->second;

	obj->set_duration(0);
	obj->set_time_begin(0);
	obj->set_time_end(0);

	WorldMap::update_map_object(virtual_shared_from_this<MapObject>(), false);

	const auto buff_data = Data::Buff::get(buff_id);
	if(buff_data){
		try {
			recalculate_attributes(true);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
		}
	}
}

std::uint64_t MapObject::get_resource_amount_carriable() const {
	PROFILE_ME;

	const auto map_object_type_id = get_map_object_type_id();
	const auto map_object_type_data = Data::MapObjectTypeBattalion::require(map_object_type_id);
	const auto carriage_bonus = get_attribute(AttributeIds::ID_CARRIAGE_BONUS) / 1000.0;
	const auto carriage_add = get_attribute(AttributeIds::ID_CARRIAGE_ADD) / 1000.0;
	const auto soldier_count = static_cast<std::uint64_t>(get_attribute(AttributeIds::ID_SOLDIER_COUNT));
	return static_cast<std::uint64_t>(std::max<double>(0,
		std::floor((map_object_type_data->resource_carriable * (1 + carriage_bonus) + carriage_add) * soldier_count + 0.001)));
}
std::uint64_t MapObject::get_resource_amount_carried() const {
	PROFILE_ME;

	std::uint64_t amount_total = 0;
	for(auto it = m_attributes.begin(); it != m_attributes.end(); ++it){
		const auto attribute_id = it->first;
		const auto value = it->second->get_value();
		if(value <= 0){
			continue;
		}
		const auto resource_data = Data::CastleResource::get_by_carried_attribute_id(attribute_id);
		if(!resource_data){
			continue;
		}
		const auto unit_weight = resource_data->unit_weight;
		if(unit_weight <= 0){
			continue;
		}
		amount_total = saturated_add(amount_total, static_cast<std::uint64_t>(value * unit_weight));
	}
	return amount_total;
}
std::uint64_t MapObject::load_resource(ResourceId resource_id, std::uint64_t amount_to_add, bool ignore_limit, bool use_alt_id){
	PROFILE_ME;

	const auto resource_data = Data::CastleResource::require(resource_id);
	const auto unit_weight = resource_data->unit_weight;
	if(unit_weight <= 0){
		LOG_EMPERY_CENTER_WARNING("Resource does not have a positive unit weight: resource_id = ", resource_id);
		DEBUG_THROW(Exception, sslit("Resource does not have a positive unit weight"));
	}
	const auto attribute_id = !use_alt_id ? resource_data->carried_attribute_id : resource_data->carried_alt_attribute_id;
	if(!attribute_id){
		LOG_EMPERY_CENTER_WARNING("Resource is not harvestable: resource_id = ", resource_id, ", use_alt_id = ", use_alt_id);
		DEBUG_THROW(Exception, sslit("Resource is not harvestable"));
	}

	auto amount_added = amount_to_add;
	if(!ignore_limit){
		const auto resource_amount_carriable = get_resource_amount_carriable();
		const auto resource_amount_carried = get_resource_amount_carried();
		const auto capacity_remaining = saturated_sub(resource_amount_carriable, resource_amount_carried);
		const auto amount_addable = static_cast<std::uint64_t>(std::ceil(capacity_remaining / unit_weight - 0.001));
		if(amount_added > amount_addable){
			amount_added = amount_addable;
		}
		LOG_EMPERY_CENTER_DEBUG("map_object_type_id = ", get_map_object_type_id(),
			", resource_amount_carriable = ", resource_amount_carriable, ", resource_amount_carried = ", resource_amount_carried,
			", capacity_remaining = ", capacity_remaining, ", unit_weight = ", unit_weight,
			", amount_addable = ", amount_addable, ", amount_added = ", amount_added);
	}

	boost::container::flat_map<AttributeId, std::int64_t> modifiers;
	modifiers[attribute_id] = static_cast<std::int64_t>(checked_add(static_cast<std::uint64_t>(get_attribute(attribute_id)), amount_added));
	set_attributes(std::move(modifiers));

	return amount_to_add;
}
void MapObject::unload_resources(const boost::shared_ptr<Castle> &castle){
	PROFILE_ME;

	const auto map_object_uuid = get_map_object_uuid();
	const auto map_object_uuid_head = Poseidon::load_be(reinterpret_cast<const std::uint64_t &>(map_object_uuid.get()[0]));

	boost::container::flat_map<ResourceId, std::pair<std::uint64_t, std::uint64_t>> resources_to_add; // <total, alt>
	std::vector<boost::shared_ptr<MySql::Center_MapObjectAttribute>> attribute_objs;
	for(auto it = m_attributes.begin(); it != m_attributes.end(); ++it){
		const auto attribute_id = it->first;
		const auto &obj = it->second;
		const auto value = obj->get_value();
		if(value <= 0){
			continue;
		}
		const auto delta_amount = static_cast<std::uint64_t>(value);
		const auto resource_data = Data::CastleResource::get_by_carried_attribute_id(attribute_id);
		if(!resource_data){
			continue;
		}

		auto &pair = resources_to_add[resource_data->resource_id];
		pair.first = checked_add(pair.first, delta_amount);
		if(attribute_id != resource_data->carried_attribute_id){
			pair.second += delta_amount;
		}
		attribute_objs.emplace_back(obj);
	}

	std::vector<ResourceTransactionElement> transaction;
	transaction.reserve(resources_to_add.size());
	for(auto it = resources_to_add.begin(); it != resources_to_add.end(); ++it){
		const auto resource_id = it->first;
		const auto capacity_remaining = saturated_sub(castle->get_warehouse_capacity(resource_id),
		                                              castle->get_resource(resource_id).amount);
		const auto amount_to_add_total = std::min(it->second.first, capacity_remaining);
		const auto amount_to_add_normal = saturated_sub(amount_to_add_total, it->second.second);
		if(amount_to_add_normal != 0){
			transaction.emplace_back(ResourceTransactionElement::OP_ADD, resource_id, amount_to_add_normal,
				ReasonIds::ID_BATTALION_UNLOAD, map_object_uuid_head, 0, 0);
		}
		const auto amount_to_add_alt = saturated_sub(amount_to_add_total, amount_to_add_normal);
		if(amount_to_add_alt != 0){
			transaction.emplace_back(ResourceTransactionElement::OP_ADD, resource_id, amount_to_add_alt,
				ReasonIds::ID_BATTALION_UNLOAD_CRATE, map_object_uuid_head, 0, 0);
		}
	}
	castle->commit_resource_transaction(transaction,
		[&]{
			for(auto it = attribute_objs.begin(); it != attribute_objs.end(); ++it){
				const auto &obj = *it;
				obj->set_value(0);
			}
		});
}

void MapObject::set_action(unsigned action, std::string action_param){
	PROFILE_ME;

	m_action       = action;
	m_action_param = std::move(action_param);

	WorldMap::update_map_object(virtual_shared_from_this<MapObject>(), false);
}

bool MapObject::is_virtually_removed() const {
	return has_been_deleted();
}
void MapObject::synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	if(is_virtually_removed()){
		Msg::SC_MapObjectRemoved msg;
		msg.object_uuid        = get_map_object_uuid().str();
		msg.object_type_id     = get_map_object_type_id().get();
		session->send(msg);
	} else {
		const auto utc_now = Poseidon::get_utc_time();

		const auto owner_uuid = get_owner_uuid();
		if(owner_uuid){
			AccountMap::cached_synchronize_account_with_player_all(owner_uuid, session);
		}

		Msg::SC_MapObjectInfo msg;
		msg.object_uuid        = get_map_object_uuid().str();
		msg.object_type_id     = get_map_object_type_id().get();
		msg.owner_uuid         = owner_uuid.str();
		msg.parent_object_uuid = get_parent_object_uuid().str();
		msg.name               = get_name();
		msg.x                  = get_coord().x();
		msg.y                  = get_coord().y();
		msg.attributes.reserve(m_attributes.size());
		for(auto it = m_attributes.begin(); it != m_attributes.end(); ++it){
			auto &attribute = *msg.attributes.emplace(msg.attributes.end());
			attribute.attribute_id = it->first.get();
			attribute.value        = it->second->get_value();
		}
		msg.garrisoned         = is_garrisoned();
		msg.action             = get_action();
		msg.param              = get_action_param();
		msg.buffs.reserve(m_buffs.size());
		for(auto it = m_buffs.begin(); it != m_buffs.end(); ++it){
			auto &buff = *msg.buffs.emplace(msg.buffs.end());
			buff.buff_id        = it->first.get();
			buff.duration       = it->second->get_duration();
			buff.time_remaining = saturated_sub(it->second->get_time_end(), utc_now);
		}
		session->send(msg);

		synchronize_with_player_additional(session);
	}
}
void MapObject::synchronize_with_cluster(const boost::shared_ptr<ClusterSession> &cluster) const {
	PROFILE_ME;

	if(has_been_deleted()){
		Msg::SK_MapRemoveMapObject msg;
		msg.map_object_uuid = get_map_object_uuid().str();
		cluster->send(msg);
	} else {
		Msg::SK_MapAddMapObject msg;
		msg.map_object_uuid    = get_map_object_uuid().str();
		msg.stamp              = m_stamp;
		msg.map_object_type_id = get_map_object_type_id().get();
		msg.owner_uuid         = get_owner_uuid().str();
		msg.parent_object_uuid = get_parent_object_uuid().str();
		msg.garrisoned         = is_garrisoned();
		msg.x                  = get_coord().x();
		msg.y                  = get_coord().y();
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
		cluster->send(msg);
	}
}

}
