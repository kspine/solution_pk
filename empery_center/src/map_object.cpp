#include "precompiled.hpp"
#include "map_object.hpp"
#include "mysql/map_object.hpp"
#include "attribute_ids.hpp"

namespace EmperyCenter {

MapObject::MapObject(MapObjectTypeId map_object_type_id, const AccountUuid &owner_uuid, std::string name){
	m_obj = boost::make_shared<MySql::Center_MapObject>(
		Poseidon::Uuid::random(), map_object_type_id.get(), owner_uuid.get(), std::move(name), Poseidon::get_utc_time(), false);
	m_obj->async_save(true);
}
MapObject::MapObject(boost::shared_ptr<MySql::Center_MapObject> obj,
	const std::vector<boost::shared_ptr<MySql::Center_MapObjectAttribute>> &attributes)
{
	m_obj = std::move(obj);
	for(auto it = attributes.begin(); it != attributes.end(); ++it){
		m_attributes.emplace(AttributeId((*it)->get_map_object_attr_id()), *it);
	}
}
MapObject::~MapObject(){
}

void MapObject::delete_from_game(){
	PROFILE_ME;
	LOG_EMPERY_CENTER_DEBUG("Deleting map object from game: map_object_uuid = ", get_map_object_uuid());

	m_obj->set_deleted(true);
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
const std::string &MapObject::get_name() const {
	return m_obj->unlocked_get_name();
}
void MapObject::set_name(std::string name){
	m_obj->set_name(std::move(name));
}
boost::uint64_t MapObject::get_created_time() const {
	return m_obj->get_created_time();
}
bool MapObject::has_been_deleted() const {
	return m_obj->get_deleted();
}

void MapObject::get_attributes(boost::container::flat_map<AttributeId, boost::int64_t> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_attributes.size());
	for(auto it = m_attributes.begin(); it != m_attributes.end(); ++it){
		ret[it->first] = it->second->get_value();
	}
}
boost::int64_t MapObject::get_attribute(AttributeId map_object_attr_id) const {
	PROFILE_ME;

	const auto it = m_attributes.find(map_object_attr_id);
	if(it == m_attributes.end()){
		return 0;
	}
	return it->second->get_value();
}
void MapObject::set_attribute(AttributeId map_object_attr_id, boost::int64_t value){
	PROFILE_ME;

	auto it = m_attributes.find(map_object_attr_id);
	if(it == m_attributes.end()){
		auto obj = boost::make_shared<MySql::Center_MapObjectAttribute>(
			m_obj->unlocked_get_map_object_uuid(), map_object_attr_id.get(), value);
		obj->async_save(true);
		it = m_attributes.emplace_hint(it, map_object_attr_id, std::move(obj));
	} else {
		it->second->set_value(value);
	}
}

Coord MapObject::get_coord() const {
	return Coord(get_attribute(AttributeIds::ID_COORD_X), get_attribute(AttributeIds::ID_COORD_Y));
}

}
