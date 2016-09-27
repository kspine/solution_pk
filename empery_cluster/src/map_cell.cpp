#include "precompiled.hpp"
#include "map_cell.hpp"
#include "map_utilities.hpp"
#include "map_object.hpp"
#include "data/map.hpp"
#include "singletons/world_map.hpp"
#include "buff_ids.hpp"

namespace EmperyCluster {

namespace {
	void fill_buff_info(MapCell::BuffInfo &dest, const MapCell::BuffInfo &src){
		PROFILE_ME;

		dest.buff_id    = src.buff_id;
		dest.duration   = src.duration;
		dest.time_begin = src.time_begin;
		dest.time_end   = src.time_end;
	}
}

MapCell::MapCell(Coord coord, MapObjectUuid parent_object_uuid, AccountUuid owner_uuid,LegionUuid legion_uuid,
    bool acceleration_card_applied,
	ItemId ticket_item_id,ResourceId production_resource_id,std::uint64_t resource_amount,
	boost::container::flat_map<AttributeId, std::int64_t> attributes,
	boost::container::flat_map<BuffId, BuffInfo> buffs,
	MapObjectUuid occupier_object_uuid,AccountUuid occupier_owner_uuid)
	: m_coord(coord)
	, m_parent_object_uuid(parent_object_uuid), m_owner_uuid(owner_uuid),m_legion_uuid(legion_uuid), m_acceleration_card_applied(acceleration_card_applied)
	, m_ticket_item_id(ticket_item_id),m_production_resource_id(production_resource_id),m_resource_amount(resource_amount)
	, m_attributes(std::move(attributes)), m_buffs(std::move(buffs))
	, m_occupier_object_uuid(occupier_object_uuid), m_occupier_owner_uuid(occupier_owner_uuid)
{
}
MapCell::~MapCell(){
}

std::int64_t MapCell::get_attribute(AttributeId map_object_attr_id) const {
	PROFILE_ME;

	const auto it = m_attributes.find(map_object_attr_id);
	if(it == m_attributes.end()){
		return 0;
	}
	return it->second;
}
void MapCell::get_attributes(boost::container::flat_map<AttributeId, std::int64_t> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_attributes.size());
	for(auto it = m_attributes.begin(); it != m_attributes.end(); ++it){
		ret[it->first] = it->second;
	}
}

MapCell::BuffInfo MapCell::get_buff(BuffId buff_id) const{
	PROFILE_ME;

	BuffInfo info = { };
	info.buff_id = buff_id;
	const auto it = m_buffs.find(buff_id);
	if(it == m_buffs.end()){
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
	const auto time_end = it->second.time_end;
	if(time_end == 0){
		return false;
	}
	const auto utc_now = Poseidon::get_utc_time();
	return utc_now < time_end;
}
void MapCell::get_buffs(std::vector<BuffInfo> &ret) const{
	PROFILE_ME;

	ret.reserve(ret.size() + m_buffs.size());
	for(auto it = m_buffs.begin(); it != m_buffs.end(); ++it){
		BuffInfo info = { };
		fill_buff_info(info, it->second);
		ret.emplace_back(std::move(info));
	}
}
void MapCell::set_buff(BuffId buff_id, std::uint64_t time_begin, std::uint64_t duration){
	auto it = m_buffs.find(buff_id);
	if(it == m_buffs.end()){
		BuffInfo info = { };
		info.buff_id = buff_id;
		info.duration = duration;
		info.time_begin = time_begin;
		info.time_end = saturated_add(time_begin, duration);
		m_buffs.emplace(it->first, std::move(info));
	}
	it->second.duration = duration;
	it->second.time_begin = time_begin;
	it->second.time_end = saturated_add(time_begin, duration);
}
void MapCell::clear_buff(BuffId buff_id) noexcept{
	PROFILE_ME;

	const auto it = m_buffs.find(buff_id);
	if(it == m_buffs.end()){
		return;
	}
	m_buffs.erase(it);
}

bool MapCell::is_acceleration_card_applied() const{
	return m_acceleration_card_applied;
}
ItemId MapCell::get_ticket_item_id() const{
	return m_ticket_item_id;
}
ResourceId MapCell::get_production_resource_id() const{
	return m_production_resource_id;
}
std::uint64_t MapCell::get_resource_amount() const{
	return m_resource_amount;
}

MapObjectUuid MapCell::get_occupier_object_uuid() const{
	return m_occupier_object_uuid;
}
AccountUuid MapCell::get_occupier_owner_uuid() const{
	return m_occupier_owner_uuid;
}

void MapCell::on_attack(boost::shared_ptr<MapObject> attacker){
	if(attacker->get_owner_uuid() == get_owner_uuid() || attacker->get_owner_uuid() == get_occupier_owner_uuid()){
		return;
	}
	std::vector<boost::shared_ptr<MapObject>> friendly_map_objects;
	if(get_occupier_owner_uuid() != AccountUuid()){
		WorldMap::get_map_objects_by_account(friendly_map_objects,get_occupier_owner_uuid());
	}else{
		WorldMap::get_map_objects_by_account(friendly_map_objects,get_owner_uuid());
	}
	if(friendly_map_objects.empty()){
		return;
	}
	for(auto it = friendly_map_objects.begin(); it != friendly_map_objects.end(); ++it){
		auto map_object = *it;
		std::pair<long, std::string> reason;
		if(!map_object || !map_object->is_idle() || !is_in_group_view_scope(map_object)||!map_object->attacking_able(reason)){
			continue;
		}
		boost::shared_ptr<MapObject> near_enemy_object;
		if(map_object->get_new_enemy(attacker->get_owner_uuid(),near_enemy_object)){
			map_object->attack_new_target(near_enemy_object);
		}else{
			map_object->attack_new_target(attacker);
		}
	}
}

bool MapCell::is_in_group_view_scope(boost::shared_ptr<MapObject> target_object){
		PROFILE_ME;

	if(!target_object){
		return false;
	}
	const auto map_cell_ticket = Data::MapCellTicket::get(get_ticket_item_id());
	if(!map_cell_ticket){
		return false;
	}
	const std::uint64_t view_range = map_cell_ticket->range;
	const auto target_view_range = target_object->get_view_range();
	const auto troops_view_range = view_range > target_view_range ? view_range:target_view_range;
	const auto coord    = get_coord();
	const auto distance = get_distance_of_coords(coord, target_object->get_coord());

	if(distance <= troops_view_range){
		return true;
	}

	return false;
}

bool MapCell::is_in_castle_protect(){
		PROFILE_ME;
	const auto map_cell_ticket = Data::MapCellTicket::get(get_ticket_item_id());
	if(!map_cell_ticket){
		return false;
	}
	if(is_buff_in_effect(BuffIds::ID_CASTLE_PROTECTION)&&!is_buff_in_effect(BuffIds::ID_CASTLE_PROTECTION_PREPARATION)&&map_cell_ticket->protect){
		return true;
	}
	return false;
}

bool MapCell::is_in_noviciate_protect(){
		PROFILE_ME;
	const auto map_cell_ticket = Data::MapCellTicket::get(get_ticket_item_id());
	if(!map_cell_ticket){
		return false;
	}
	if(is_buff_in_effect(BuffIds::ID_NOVICIATE_PROTECTION)&&map_cell_ticket->protect){
		return true;
	}
	return false;
}


bool MapCell::is_protectable() const{
	PROFILE_ME;

	const auto map_cell_ticket = Data::MapCellTicket::get(get_ticket_item_id());
	if(!map_cell_ticket){
		return false;
	}
	return map_cell_ticket->protect;
}

}
