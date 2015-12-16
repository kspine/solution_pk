#include "precompiled.hpp"
#include "map_cell.hpp"

namespace EmperyCluster {

MapCell::MapCell(Coord coord, MapObjectUuid parent_object_uuid, AccountUuid owner_uuid,
	boost::container::flat_map<AttributeId, boost::int64_t> attributes)
	: m_coord(coord)
	, m_parent_object_uuid(parent_object_uuid), m_owner_uuid(owner_uuid), m_attributes(std::move(attributes))
{
}
MapCell::~MapCell(){
}

boost::int64_t MapCell::get_attribute(AttributeId map_object_attr_id) const {
	PROFILE_ME;

	const auto it = m_attributes.find(map_object_attr_id);
	if(it == m_attributes.end()){
		return 0;
	}
	return it->second;
}
void MapCell::get_attributes(boost::container::flat_map<AttributeId, boost::int64_t> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_attributes.size());
	for(auto it = m_attributes.begin(); it != m_attributes.end(); ++it){
		ret[it->first] = it->second;
	}
}

}
