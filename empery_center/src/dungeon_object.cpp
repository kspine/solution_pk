#include "precompiled.hpp"
#include "dungeon_object.hpp"
#include "dungeon.hpp"
#include "singletons/account_map.hpp"
#include "msg/sc_dungeon.hpp"
#include "msg/sd_dungeon.hpp"
#include "player_session.hpp"
#include "dungeon_session.hpp"
#include "singletons/dungeon_map.hpp"
#include "data/buff.hpp"
#include "singletons/world_map.hpp"
#include "map_object.hpp"

namespace EmperyCenter {

DungeonObject::DungeonObject(DungeonUuid dungeon_uuid, DungeonObjectUuid dungeon_object_uuid,
	MapObjectTypeId map_object_type_id, AccountUuid owner_uuid, std::string tag, Coord coord)
	: m_dungeon_uuid(dungeon_uuid), m_dungeon_object_uuid(dungeon_object_uuid)
	, m_map_object_type_id(map_object_type_id), m_owner_uuid(owner_uuid), m_tag(std::move(tag))
	, m_coord(coord)
{
}
DungeonObject::~DungeonObject(){
}

void DungeonObject::pump_status(){
	PROFILE_ME;

	//
}
void DungeonObject::recalculate_attributes(bool recursive){
	PROFILE_ME;

	const auto utc_now = Poseidon::get_utc_time();

	boost::container::flat_map<AttributeId, std::int64_t> modifiers;
	modifiers.reserve(64);
	const auto shadow_object_uuid = MapObjectUuid(get_dungeon_object_uuid().get());
	const auto shadow_object = WorldMap::get_map_object(shadow_object_uuid);
	if(shadow_object){
		shadow_object->recalculate_attributes(recursive);
		shadow_object->get_attributes(modifiers);
	}

	for(auto it = m_buffs.begin(); it != m_buffs.end(); ++it){
		const auto buff_id = it->first;
		const auto time_end = it->second.time_end;
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
}

void DungeonObject::set_coord_no_synchronize(Coord coord) noexcept {
	PROFILE_ME;

	if(get_coord() == coord){
		return;
	}
	m_coord = coord;
}

void DungeonObject::delete_from_game() noexcept {
	PROFILE_ME;

	m_deleted = true;

	const auto dungeon_uuid = get_dungeon_uuid();
	const auto dungeon = DungeonMap::get(dungeon_uuid);
	if(dungeon){
		dungeon->update_object(virtual_shared_from_this<DungeonObject>(), false);
	}
}

std::int64_t DungeonObject::get_attribute(AttributeId attribute_id) const {
	PROFILE_ME;

	const auto it = m_attributes.find(attribute_id);
	if(it == m_attributes.end()){
		return 0;
	}
	return it->second;
}
void DungeonObject::get_attributes(boost::container::flat_map<AttributeId, std::int64_t> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_attributes.size());
	for(auto it = m_attributes.begin(); it != m_attributes.end(); ++it){
		ret[it->first] = it->second;
	}
}
void DungeonObject::set_attributes(boost::container::flat_map<AttributeId, std::int64_t> modifiers){
	PROFILE_ME;

	for(auto it = modifiers.begin(); it != modifiers.end(); ++it){
		const auto obj_it = m_attributes.find(it->first);
		if(obj_it == m_attributes.end()){
			m_attributes.emplace(it->first, 0);
		}
	}

	bool dirty = false;
	for(auto it = modifiers.begin(); it != modifiers.end(); ++it){
		auto &value = m_attributes.at(it->first);
		if(value == it->second){
			continue;
		}
		value = it->second;
		++dirty;
	}
	if(!dirty){
		return;
	}

	const auto dungeon_uuid = get_dungeon_uuid();
	const auto dungeon = DungeonMap::get(dungeon_uuid);
	if(dungeon){
		dungeon->update_object(virtual_shared_from_this<DungeonObject>(), false);
	}
}

DungeonObject::BuffInfo DungeonObject::get_buff(BuffId buff_id) const {
	PROFILE_ME;

	BuffInfo info = { };
	info.buff_id = buff_id;
	const auto it = m_buffs.find(buff_id);
	if(it == m_buffs.end()){
		return info;
	}
	if(it->second.time_end == 0){
		return info;
	}
	info = it->second;
	return info;
}
bool DungeonObject::is_buff_in_effect(BuffId buff_id) const {
	PROFILE_ME;

	const auto it = m_buffs.find(buff_id);
	if(it == m_buffs.end()){
		return false;
	}
	const auto time_end = it->second.time_end;
	if(time_end == 0){
		return false;
	}
	const auto utc_now = Poseidon::get_utc_time();
	return utc_now < time_end;
}
void DungeonObject::get_buffs(std::vector<DungeonObject::BuffInfo> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_buffs.size());
	for(auto it = m_buffs.begin(); it != m_buffs.end(); ++it){
		if(it->second.time_end == 0){
			continue;
		}
		ret.emplace_back(it->second);
	}
}
void DungeonObject::set_buff(BuffId buff_id, std::uint64_t duration){
	PROFILE_ME;

	const auto utc_now = Poseidon::get_utc_time();
	set_buff(buff_id, utc_now, duration);
}
void DungeonObject::set_buff(BuffId buff_id, std::uint64_t time_begin, std::uint64_t duration){
	PROFILE_ME;

	auto it = m_buffs.find(buff_id);
	if(it == m_buffs.end()){
		it = m_buffs.emplace(buff_id, BuffInfo()).first;
	}
	auto &elem = it->second;

	elem.duration = duration;
	elem.time_begin = time_begin;
	elem.time_end = saturated_add(time_begin, duration);

	const auto dungeon_uuid = get_dungeon_uuid();
	const auto dungeon = DungeonMap::get(dungeon_uuid);
	if(dungeon){
		dungeon->update_object(virtual_shared_from_this<DungeonObject>(), false);
	}

	const auto buff_data = Data::Buff::get(buff_id);
	if(buff_data){
		try {
			recalculate_attributes(true);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
		}
	}
}
void DungeonObject::accumulate_buff(BuffId buff_id, std::uint64_t delta_duration){
	PROFILE_ME;

	auto it = m_buffs.find(buff_id);
	if(it == m_buffs.end()){
		it = m_buffs.emplace(buff_id, BuffInfo()).first;
	}
	auto &elem = it->second;
	const auto utc_now = Poseidon::get_utc_time();

	const auto old_time_begin = elem.time_begin, old_time_end = elem.time_end;
	std::uint64_t new_time_begin, new_time_end;
	if(utc_now < old_time_end){
		new_time_begin = old_time_begin;
		new_time_end = saturated_add(old_time_end, delta_duration);
	} else {
		new_time_begin = utc_now;
		new_time_end = saturated_add(utc_now, delta_duration);
	}
	elem.duration = saturated_sub(new_time_end, new_time_begin);
	elem.time_begin = new_time_begin;
	elem.time_end = new_time_end;

	const auto dungeon_uuid = get_dungeon_uuid();
	const auto dungeon = DungeonMap::get(dungeon_uuid);
	if(dungeon){
		dungeon->update_object(virtual_shared_from_this<DungeonObject>(), false);
	}

	const auto buff_data = Data::Buff::get(buff_id);
	if(buff_data){
		try {
			recalculate_attributes(true);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
		}
	}
}
void DungeonObject::clear_buff(BuffId buff_id) noexcept {
	PROFILE_ME;

	const auto it = m_buffs.find(buff_id);
	if(it == m_buffs.end()){
		return;
	}
	auto &elem = it->second;

	elem.duration = 0;
	elem.time_begin = 0;
	elem.time_end = 0;

	const auto dungeon_uuid = get_dungeon_uuid();
	const auto dungeon = DungeonMap::get(dungeon_uuid);
	if(dungeon){
		dungeon->update_object(virtual_shared_from_this<DungeonObject>(), false);
	}

	const auto buff_data = Data::Buff::get(buff_id);
	if(buff_data){
		try {
			recalculate_attributes(true);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
		}
	}
}

void DungeonObject::set_action(unsigned action, std::string action_param){
	PROFILE_ME;

	m_action       = action;
	m_action_param = std::move(action_param);

	const auto dungeon_uuid = get_dungeon_uuid();
	const auto dungeon = DungeonMap::get(dungeon_uuid);
	if(dungeon){
		dungeon->update_object(virtual_shared_from_this<DungeonObject>(), false);
	}
}

bool DungeonObject::is_virtually_removed() const {
	return has_been_deleted();
}
void DungeonObject::synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	if(is_virtually_removed()){
		Msg::SC_DungeonObjectRemoved msg;
		msg.dungeon_uuid        = get_dungeon_uuid().str();
		msg.dungeon_object_uuid = get_dungeon_object_uuid().str();
		msg.map_object_type_id  = get_map_object_type_id().get();
		session->send(msg);
	} else {
		const auto utc_now = Poseidon::get_utc_time();

		const auto owner_uuid = get_owner_uuid();
		if(owner_uuid){
			AccountMap::cached_synchronize_account_with_player_all(owner_uuid, session);
		}

		Msg::SC_DungeonObjectInfo msg;
		msg.dungeon_uuid        = get_dungeon_uuid().str();
		msg.dungeon_object_uuid = get_dungeon_object_uuid().str();
		msg.map_object_type_id  = get_map_object_type_id().get();
		msg.owner_uuid         = owner_uuid.str();
		msg.x                  = get_coord().x();
		msg.y                  = get_coord().y();
		msg.attributes.reserve(m_attributes.size());
		for(auto it = m_attributes.begin(); it != m_attributes.end(); ++it){
			auto &attribute = *msg.attributes.emplace(msg.attributes.end());
			attribute.attribute_id = it->first.get();
			attribute.value        = it->second;
		}
		msg.action             = get_action();
		msg.param              = get_action_param();
		msg.buffs.reserve(m_buffs.size());
		for(auto it = m_buffs.begin(); it != m_buffs.end(); ++it){
			auto &buff = *msg.buffs.emplace(msg.buffs.end());
			buff.buff_id        = it->first.get();
			buff.duration       = it->second.duration;
			buff.time_remaining = saturated_sub(it->second.time_end, utc_now);
		}
		session->send(msg);
	}
}
void DungeonObject::synchronize_with_dungeon_server(const boost::shared_ptr<DungeonSession> &server) const {
	PROFILE_ME;

	if(has_been_deleted()){
		Msg::SD_DungeonObjectRemoved msg;
		msg.dungeon_uuid        = get_dungeon_uuid().str();
		msg.dungeon_object_uuid = get_dungeon_object_uuid().str();
		server->send(msg);
	} else {
		Msg::SD_DungeonObjectInfo msg;
		msg.dungeon_uuid        = get_dungeon_uuid().str();
		msg.dungeon_object_uuid = get_dungeon_object_uuid().str();
		msg.map_object_type_id  = get_map_object_type_id().get();
		msg.owner_uuid          = get_owner_uuid().str();
		msg.x                   = get_coord().x();
		msg.y                   = get_coord().y();
		msg.attributes.reserve(m_attributes.size());
		for(auto it = m_attributes.begin(); it != m_attributes.end(); ++it){
			auto &attribute = *msg.attributes.emplace(msg.attributes.end());
			attribute.attribute_id = it->first.get();
			attribute.value        = it->second;
		}
		msg.buffs.reserve(m_buffs.size());
		for(auto it = m_buffs.begin(); it != m_buffs.end(); ++it){
			auto &buff = *msg.buffs.emplace(msg.buffs.end());
			buff.buff_id    = it->first.get();
			buff.time_begin = it->second.time_begin;
			buff.time_end   = it->second.time_end;
		}
		server->send(msg);
	}
}

}
