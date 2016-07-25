#include "precompiled.hpp"
#include "dungeon_object.hpp"
#include "dungeon.hpp"
#include "singletons/dungeon_map.hpp"

namespace EmperyDungeon {

DungeonObject::DungeonObject(DungeonUuid dungeon_uuid, DungeonObjectUuid dungeon_object_uuid,
	MapObjectTypeId map_object_type_id, AccountUuid owner_uuid, Coord coord)
	: m_dungeon_uuid(dungeon_uuid), m_dungeon_object_uuid(dungeon_object_uuid)
	, m_map_object_type_id(map_object_type_id), m_owner_uuid(owner_uuid)
	, m_coord(coord)
{
}
DungeonObject::~DungeonObject(){
}

void DungeonObject::pump_status(){
	PROFILE_ME;

	//
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

DungeonObject::BuffInfo DungeonObject::get_buff(BuffId buff_id) const{
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

bool DungeonObject::is_buff_in_effect(BuffId buff_id) const{
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

void DungeonObject::get_buffs(std::vector<DungeonObject::BuffInfo> &ret) const{
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
		BuffInfo info = { };
		info.buff_id = buff_id;
		info.duration = duration;
		info.time_begin = time_begin;
		info.time_end = saturated_add(time_begin, duration);
		it = m_buffs.emplace(buff_id, std::move(info)).first;
	}
	auto &info = it->second;
	info.duration = duration;
	info.time_begin = time_begin;
	info.time_end = saturated_add(time_begin, duration);
}

void DungeonObject::clear_buff(BuffId buff_id) noexcept{
	PROFILE_ME;

	const auto it = m_buffs.find(buff_id);
	if(it == m_buffs.end()){
		return;
	}
	m_buffs.erase(it);
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

}
