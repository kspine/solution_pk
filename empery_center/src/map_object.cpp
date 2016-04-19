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
#include "data/castle.hpp"

namespace EmperyCenter {

MapObject::MapObject(MapObjectUuid map_object_uuid, MapObjectTypeId map_object_type_id, AccountUuid owner_uuid, MapObjectUuid parent_object_uuid,
	std::string name, Coord coord, std::uint64_t created_time, bool garrisoned)
	: m_obj(
		[&]{
			auto obj = boost::make_shared<MySql::Center_MapObject>(map_object_uuid.get(), map_object_type_id.get(),
				owner_uuid.get(), parent_object_uuid.get(), std::move(name), coord.x(), coord.y(), created_time, false, garrisoned);
			obj->async_save(true);
			return obj;
		}())
{
}
MapObject::MapObject(boost::shared_ptr<MySql::Center_MapObject> obj,
	const std::vector<boost::shared_ptr<MySql::Center_MapObjectAttribute>> &attributes)
	: m_obj(std::move(obj))
{
	for(auto it = attributes.begin(); it != attributes.end(); ++it){
		m_attributes.emplace(AttributeId((*it)->get_attribute_id()), *it);
	}
}
MapObject::~MapObject(){
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
		recalculate_attributes();
	}
}
void MapObject::recalculate_attributes(){
	PROFILE_ME;

	boost::container::flat_map<AttributeId, std::int64_t> modifiers;
	modifiers.reserve(32);

	const auto map_object_type_id = get_map_object_type_id();
	const auto map_object_data = Data::MapObjectTypeAbstract::require(map_object_type_id);

	boost::shared_ptr<MapObject> parent_object;
	const auto parent_object_uuid = get_parent_object_uuid();
	if(parent_object_uuid){
		parent_object = WorldMap::get_map_object(parent_object_uuid);
	}
	if(parent_object){
		modifiers.emplace_hint(modifiers.end(), AttributeIds::ID_ATTACK_BONUS,                     0);
		modifiers.emplace_hint(modifiers.end(), AttributeIds::ID_DEFENSE_BONUS,                    0);
		modifiers.emplace_hint(modifiers.end(), AttributeIds::ID_DODGING_RATIO_BONUS,              0);
		modifiers.emplace_hint(modifiers.end(), AttributeIds::ID_CRITICAL_DAMAGE_RATIO_BONUS,      0);
		modifiers.emplace_hint(modifiers.end(), AttributeIds::ID_CRITICAL_DAMAGE_MULTIPLIER_BONUS, 0);
		modifiers.emplace_hint(modifiers.end(), AttributeIds::ID_ATTACK_RANGE_BONUS,               0);
		modifiers.emplace_hint(modifiers.end(), AttributeIds::ID_SIGHT_RANGE_BONUS,                0);
		modifiers.emplace_hint(modifiers.end(), AttributeIds::ID_RATE_OF_FIRE_BONUS,               0);
		modifiers.emplace_hint(modifiers.end(), AttributeIds::ID_SPEED_BONUS,                      0);

		LOG_EMPERY_CENTER_DEBUG("Updating attributes from castle: map_object_uuid = ", get_map_object_uuid(),
			", parent_object_uuid = ", parent_object_uuid);

		std::vector<boost::shared_ptr<const Data::MapObjectTypeAttributeBonus>> attribute_bonus_applicable_data;
		Data::MapObjectTypeAttributeBonus::get_applicable(attribute_bonus_applicable_data,
			Data::MapObjectTypeAttributeBonus::AKT_ALL, 0);
		Data::MapObjectTypeAttributeBonus::get_applicable(attribute_bonus_applicable_data,
			Data::MapObjectTypeAttributeBonus::AKT_CHASSIS_ID, map_object_data->map_object_chassis_id.get());
		Data::MapObjectTypeAttributeBonus::get_applicable(attribute_bonus_applicable_data,
			Data::MapObjectTypeAttributeBonus::AKT_WEAPON_ID, map_object_data->map_object_weapon_id.get());
		Data::MapObjectTypeAttributeBonus::get_applicable(attribute_bonus_applicable_data,
			Data::MapObjectTypeAttributeBonus::AKT_MAP_OBJECT_TYPE_ID, map_object_data->map_object_type_id.get());
		for(auto it = attribute_bonus_applicable_data.begin(); it != attribute_bonus_applicable_data.end(); ++it){
			const auto &attribute_bonus_data = *it;
			const auto tech_attribute_id = attribute_bonus_data->tech_attribute_id;
			const auto bonus_attribute_id = attribute_bonus_data->bonus_attribute_id;
			const auto tech_attribute_value = parent_object->get_attribute(tech_attribute_id);
			LOG_EMPERY_CENTER_TRACE("> Applying attribute bonus: tech_attribute_id = ", tech_attribute_id,
				", bonus_attribute_id = ", bonus_attribute_id, ", tech_attribute_value = ", tech_attribute_value);
			modifiers[bonus_attribute_id] += tech_attribute_value;
		}
	}

	set_attributes(std::move(modifiers));

	const auto hi_res_now = Poseidon::get_hi_res_mono_clock();
	m_last_updated_time = hi_res_now;
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

	if(get_coord() == coord){
		return;
	}
	m_obj->set_x(coord.x());
	m_obj->set_y(coord.y());

	WorldMap::update_map_object(virtual_shared_from_this<MapObject>(), false);

	const auto session = PlayerSessionMap::get(get_owner_uuid());
	if(session){
		try {
			synchronize_with_player(session);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
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

	const auto session = PlayerSessionMap::get(get_owner_uuid());
	if(session){
		try {
			synchronize_with_player(session);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
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

	WorldMap::update_map_object(virtual_shared_from_this<MapObject>(), false);

	const auto session = PlayerSessionMap::get(get_owner_uuid());
	if(session){
		try {
			synchronize_with_player(session);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
}

bool MapObject::is_garrisoned() const {
	return m_obj->get_garrisoned();
}
void MapObject::set_garrisoned(bool garrisoned){
	PROFILE_ME;

	if(is_garrisoned() == garrisoned){
		return;
	}
	m_obj->set_garrisoned(garrisoned);

	WorldMap::update_map_object(virtual_shared_from_this<MapObject>(), false);

	const auto session = PlayerSessionMap::get(get_owner_uuid());
	if(session){
		try {
			synchronize_with_player(session);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
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
			auto obj = boost::make_shared<MySql::Center_MapObjectAttribute>(
				get_map_object_uuid().get(), it->first.get(), 0);
			obj->async_save(true);
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

	const auto session = PlayerSessionMap::get(get_owner_uuid());
	if(session){
		try {
			synchronize_with_player(session);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
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
		amount_total = saturated_add(amount_total, static_cast<std::uint64_t>(value));
	}
	return amount_total;
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
		const auto owner_uuid = get_owner_uuid();
		if(owner_uuid){
			AccountMap::cached_synchronize_account_with_player(owner_uuid, session);
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
		session->send(msg);
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
		cluster->send(msg);
	}
}

}
