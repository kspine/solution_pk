#include "precompiled.hpp"
#include "map_cell.hpp"
#include "map_object.hpp"
#include "mysql/map_cell.hpp"
#include "attribute_ids.hpp"
#include "singletons/world_map.hpp"
#include "player_session.hpp"
#include "msg/sc_map.hpp"

namespace EmperyCenter {

MapCell::MapCell(Coord coord)
	: m_obj([&]{
		auto obj = boost::make_shared<MySql::Center_MapCell>(coord.x(), coord.y(), Poseidon::Uuid());
		obj->async_save(true);
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

	// 无事可做。
}
void MapCell::synchronize_with_client(const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	Msg::SC_MapCellInfo msg;
	msg.x                  = get_coord().x();
	msg.y                  = get_coord().y();
	msg.parent_object_uuid = get_parent_object_uuid().str();
	msg.owner_uuid         = get_owner_uuid().str();
	msg.attributes.reserve(m_attributes.size());
	for(auto it = m_attributes.begin(); it != m_attributes.end(); ++it){
		msg.attributes.emplace_back();
		auto &attribute = msg.attributes.back();
		attribute.attribute_id = it->first.get();
		attribute.value        = it->second->get_value();
	}
	// TODO buff
	session->send(msg);
}

Coord MapCell::get_coord() const {
	return Coord(m_obj->get_x(), m_obj->get_y());
}

MapObjectUuid MapCell::get_parent_object_uuid() const {
	return MapObjectUuid(m_obj->unlocked_get_parent_object_uuid());
}
AccountUuid MapCell::get_owner_uuid() const {
	PROFILE_ME;

	const auto parent_object = WorldMap::get_map_object(get_parent_object_uuid());
	if(!parent_object){
		return AccountUuid();
	}
	return parent_object->get_owner_uuid();
}
void MapCell::set_parent_object_uuid(MapObjectUuid parent_object_uuid){
	m_obj->set_parent_object_uuid(parent_object_uuid.get());
}

boost::int64_t MapCell::get_attribute(AttributeId attribute_id) const {
	PROFILE_ME;

	const auto it = m_attributes.find(attribute_id);
	if(it == m_attributes.end()){
		return 0;
	}
	return it->second->get_value();
}
void MapCell::get_attributes(boost::container::flat_map<AttributeId, boost::int64_t> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_attributes.size());
	for(auto it = m_attributes.begin(); it != m_attributes.end(); ++it){
		ret[it->first] = it->second->get_value();
	}
}
void MapCell::set_attributes(const boost::container::flat_map<AttributeId, boost::int64_t> &modifiers){
	PROFILE_ME;

	for(auto it = modifiers.begin(); it != modifiers.end(); ++it){
		const auto obj_it = m_attributes.find(it->first);
		if(obj_it == m_attributes.end()){
			auto obj = boost::make_shared<MySql::Center_MapCellAttribute>(m_obj->get_x(), m_obj->get_y(),
				it->first.get(), 0);
			// obj->async_save(true);
			obj->enable_auto_saving();
			m_attributes.emplace(it->first, std::move(obj));
		}
	}
	for(auto it = modifiers.begin(); it != modifiers.end(); ++it){
		const auto &obj = m_attributes.at(it->first);
		obj->set_value(it->second);
	}

	WorldMap::update_map_cell(virtual_shared_from_this<MapCell>(), false);
}

}
