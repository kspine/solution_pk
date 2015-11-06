#include "precompiled.hpp"
#include "map_object.hpp"
#include "mysql/map_object.hpp"
#include "mysql/map_object_attribute.hpp"
#include "attribute_ids.hpp"

namespace EmperyCenter {

MapObject::MapObject(MapObjectTypeId mapObjectTypeId, const AccountUuid &ownerUuid){
	m_obj = boost::make_shared<MySql::Center_MapObject>(
		Poseidon::Uuid::random(), mapObjectTypeId.get(), ownerUuid.get(), Poseidon::getUtcTime(), false);
	m_obj->asyncSave(true);
}
MapObject::MapObject(boost::shared_ptr<MySql::Center_MapObject> obj,
	const std::vector<boost::shared_ptr<MySql::Center_MapObjectAttribute>> &attributes)
{
	m_obj = std::move(obj);
	for(auto it = attributes.begin(); it != attributes.end(); ++it){
		m_attributes.emplace(AttributeId((*it)->get_mapObjectAttrId()), *it);
	}
}
MapObject::~MapObject(){
}

void MapObject::deleteFromGame(){
	PROFILE_ME;
	LOG_EMPERY_CENTER_DEBUG("Deleting map object from game: mapObjectUuid = ", getMapObjectUuid());

	m_obj->set_deleted(true);
}

MapObjectUuid MapObject::getMapObjectUuid() const {
	return MapObjectUuid(m_obj->unlockedGet_mapObjectUuid());
}
MapObjectTypeId MapObject::getMapObjectTypeId() const {
	return MapObjectTypeId(m_obj->get_mapObjectTypeId());
}
AccountUuid MapObject::getOwnerUuid() const {
	return AccountUuid(m_obj->unlockedGet_ownerUuid());
}
boost::uint64_t MapObject::getCreatedTime() const {
	return m_obj->get_createdTime();
}
bool MapObject::hasBeenDeleted() const {
	return m_obj->get_deleted();
}

void MapObject::getAttributes(boost::container::flat_map<AttributeId, boost::int64_t> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_attributes.size());
	for(auto it = m_attributes.begin(); it != m_attributes.end(); ++it){
		ret[it->first] = it->second->get_value();
	}
}
boost::int64_t MapObject::getAttribute(AttributeId mapObjectAttrId) const {
	PROFILE_ME;

	const auto it = m_attributes.find(mapObjectAttrId);
	if(it == m_attributes.end()){
		return 0;
	}
	return it->second->get_value();
}
void MapObject::setAttribute(AttributeId mapObjectAttrId, boost::int64_t value){
	PROFILE_ME;

	auto it = m_attributes.find(mapObjectAttrId);
	if(it == m_attributes.end()){
		auto obj = boost::make_shared<MySql::Center_MapObjectAttribute>(
			m_obj->unlockedGet_mapObjectUuid(), mapObjectAttrId.get(), value);
		obj->asyncSave(true);
		it = m_attributes.emplace_hint(it, mapObjectAttrId, std::move(obj));
	} else {
		it->second->set_value(value);
	}
}

Coord MapObject::getCoord() const {
	return Coord(getAttribute(AttributeIds::ID_COORD_X), getAttribute(AttributeIds::ID_COORD_Y));
}

}
