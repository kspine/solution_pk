#ifndef EMPERY_CENTER_MAP_OBJECT_HPP_
#define EMPERY_CENTER_MAP_OBJECT_HPP_

#include "abstract_data_object.hpp"
#include <boost/container/flat_map.hpp>
#include "id_types.hpp"
#include "coord.hpp"

namespace EmperyCenter {

namespace MySql {
	class Center_MapObject;
	class Center_MapObjectAttribute;
}

class PlayerSession;

class MapObject : public virtual AbstractDataObject {
private:
	boost::shared_ptr<MySql::Center_MapObject> m_obj;
	boost::container::flat_map<AttributeId,
		boost::shared_ptr<MySql::Center_MapObjectAttribute>> m_attributes;

public:
	MapObject(MapObjectUuid map_object_uuid, MapObjectTypeId map_object_type_id,
		AccountUuid owner_uuid, MapObjectUuid parent_object_uuid, std::string name, Coord coord);
	MapObject(boost::shared_ptr<MySql::Center_MapObject> obj,
		const std::vector<boost::shared_ptr<MySql::Center_MapObjectAttribute>> &attributes);
	~MapObject();

public:
	void pump_status() override;
	void synchronize_with_client(const boost::shared_ptr<PlayerSession> &session) const override;

	MapObjectUuid get_map_object_uuid() const;
	MapObjectTypeId get_map_object_type_id() const;
	AccountUuid get_owner_uuid() const;
	MapObjectUuid get_parent_object_uuid() const;

	const std::string &get_name() const;
	void set_name(std::string name);

	Coord get_coord() const;
	void set_coord(Coord coord) noexcept;

	boost::uint64_t get_created_time() const;

	bool has_been_deleted() const;
	void delete_from_game() noexcept;

	boost::int64_t get_attribute(AttributeId attribute_id) const;
	void get_attributes(boost::container::flat_map<AttributeId, boost::int64_t> &ret) const;
	void set_attributes(const boost::container::flat_map<AttributeId, boost::int64_t> &modifiers);
};

}

#endif
