#include "precompiled.hpp"
#include "castle.hpp"
#include "../../empery_center/src/mysql/map_object.hpp"
#include "singletons/world_map.hpp"

namespace EmperyController {

Castle::Castle(boost::shared_ptr<EmperyCenter::MySql::Center_MapObject> obj)
	: m_obj(std::move(obj))
{
}
Castle::~Castle(){
}

MapObjectUuid Castle::get_map_object_uuid() const {
	return MapObjectUuid(m_obj->unlocked_get_map_object_uuid());
}
MapObjectTypeId Castle::get_map_object_type_id() const {
	return MapObjectTypeId(m_obj->get_map_object_type_id());
}
AccountUuid Castle::get_owner_uuid() const {
	return AccountUuid(m_obj->unlocked_get_owner_uuid());
}
MapObjectUuid Castle::get_parent_object_uuid() const {
	return MapObjectUuid(m_obj->unlocked_get_parent_object_uuid());
}

const std::string &Castle::get_name() const {
	return m_obj->unlocked_get_name();
}
Coord Castle::get_coord() const {
	return Coord(m_obj->get_x(), m_obj->get_y());
}
std::uint64_t Castle::get_created_time() const {
	return m_obj->get_created_time();
}
bool Castle::has_been_deleted() const {
	return m_obj->get_expiry_time() == 0;
}
bool Castle::is_garrisoned() const {
	return m_obj->get_garrisoned();
}

}
