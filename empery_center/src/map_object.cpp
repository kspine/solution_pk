#include "precompiled.hpp"
#include "map_object.hpp"
#include "mysql/map_object.hpp"
#include "mysql/map_object_attribute.hpp"
#include <poseidon/singletons/mysql_daemon.hpp>

namespace EmperyCenter {

MapObject::MapObject(MapObjectTypeId mapObjectTypeId, const AccountUuid &ownerUuid){
	m_obj = boost::make_shared<MySql::Center_MapObject>(
		Poseidon::Uuid::random(), mapObjectTypeId.get(), ownerUuid.get(), Poseidon::getLocalTime(), false);
	m_obj->asyncSave(true);
}
MapObject::MapObject(boost::shared_ptr<MySql::Center_MapObject> obj,
	const std::vector<boost::shared_ptr<MySql::Center_MapObjectAttribute>> &attributes)
{
	m_obj = std::move(obj);
	for(auto it = attributes.begin(); it != attributes.end(); ++it){
		m_attributes.emplace(MapObjectAttrId((*it)->get_mapObjectAttrId()), *it);
	}
}
MapObject::~MapObject(){
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
	PROFILE_ME;

	return m_obj->get_deleted();
}
void MapObject::deleteFromGame(){
	PROFILE_ME;
	LOG_EMPERY_CENTER_DEBUG("Deleting map object from game: mapObjectUuid = ", getMapObjectUuid());

	m_obj->set_deleted(true);

	m_obj->disableAutoSaving();
	for(auto it = m_attributes.begin(); it != m_attributes.end(); ++it){
		it->second->disableAutoSaving();
	}

	std::ostringstream oss;
	oss <<"DELETE `Center_MapObject`, `Center_MapObjectAttribute` "
	    <<"  FROM `Center_MapObject` LEFT JOIN `Center_MapObjectAttribute` "
	    <<"    ON `Center_MapObject`.`mapObjectUuid` = `Center_MapObjectAttribute`.`mapObjectUuid` "
	    <<"  WHERE `Center_MapObject`.`mapObjectUuid` = '" <<m_obj->unlockedGet_mapObjectUuid() <<"' ";
	Poseidon::MySqlDaemon::enqueueForBatchLoading({ }, nullptr, m_obj->getTableName(), oss.str());
}

void MapObject::getAttributes(boost::container::flat_map<MapObjectAttrId, boost::int64_t> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_attributes.size());
	for(auto it = m_attributes.begin(); it != m_attributes.end(); ++it){
		ret[it->first] = it->second->get_value();
	}
}
boost::int64_t MapObject::getAttribute(MapObjectAttrId mapObjectAttrId) const {
	PROFILE_ME;

	const auto it = m_attributes.find(mapObjectAttrId);
	if(it == m_attributes.end()){
		return 0;
	}
	return it->second->get_value();
}
void MapObject::setAttribute(MapObjectAttrId mapObjectAttrId, boost::int64_t value){
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

}
