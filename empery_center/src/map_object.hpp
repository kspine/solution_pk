#ifndef EMPERY_CENTER_MAP_OBJECT_HPP_
#define EMPERY_CENTER_MAP_OBJECT_HPP_

#include <poseidon/cxx_util.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/container/flat_map.hpp>
#include <utility>
#include "id_types.hpp"
#include "coord.hpp"

namespace EmperyCenter {

namespace MySql {
	class Center_MapObject;
	class Center_MapObjectAttribute;
}

class MapObjectMap;

class MapObject : NONCOPYABLE {
	friend MapObjectMap;

private:
	boost::shared_ptr<MySql::Center_MapObject> m_obj;
	boost::container::flat_map<AttributeId, boost::shared_ptr<MySql::Center_MapObjectAttribute>> m_attributes;

public:
	MapObject(MapObjectTypeId map_object_type_id, AccountUuid owner_uuid, std::string name);
	MapObject(boost::shared_ptr<MySql::Center_MapObject> obj,
		const std::vector<boost::shared_ptr<MySql::Center_MapObjectAttribute>> &attributes);
	virtual ~MapObject();

protected:
	void delete_from_game();

public:
	MapObjectUuid get_map_object_uuid() const;
	MapObjectTypeId get_map_object_type_id() const;
	AccountUuid get_owner_uuid() const;
	const std::string &get_name() const;
	void set_name(std::string name);
	boost::uint64_t get_created_time() const;
	bool has_been_deleted() const;

	void get_attributes(boost::container::flat_map<AttributeId, boost::int64_t> &ret) const;
	boost::int64_t get_attribute(AttributeId map_object_attr_id) const;
	void set_attribute(AttributeId map_object_attr_id, boost::int64_t value);

	Coord get_coord() const;
};

}

#endif
