#ifndef EMPERY_CENTER_MAP_CELL_HPP_
#define EMPERY_CENTER_MAP_CELL_HPP_

#include "abstract_data_object.hpp"
#include <boost/container/flat_map.hpp>
#include "id_types.hpp"
#include "coord.hpp"

namespace EmperyCenter {

namespace MySql {
	class Center_MapCell;
	class Center_MapCellAttribute;
}

class PlayerSession;

class MapCell : public virtual AbstractDataObject {
private:
	boost::shared_ptr<MySql::Center_MapCell> m_obj;
	boost::container::flat_map<AttributeId,
		boost::shared_ptr<MySql::Center_MapCellAttribute>> m_attributes;

	// buff

public:
	explicit MapCell(Coord coord);
	MapCell(boost::shared_ptr<MySql::Center_MapCell> obj,
		const std::vector<boost::shared_ptr<MySql::Center_MapCellAttribute>> &attributes);
	~MapCell();

public:
	void pump_status() override;

	void synchronize_with_client(const boost::shared_ptr<PlayerSession> &session) const;

	Coord get_coord() const;

	MapObjectUuid get_parent_object_uuid() const;
	AccountUuid get_owner_uuid() const;
	void set_parent_object_uuid(MapObjectUuid parent_object_uuid);

	boost::int64_t get_attribute(AttributeId attribute_id) const;
	void get_attributes(boost::container::flat_map<AttributeId, boost::int64_t> &ret) const;
	void set_attributes(const boost::container::flat_map<AttributeId, boost::int64_t> &modifiers);
};

inline void synchronize_map_cell_with_client(const boost::shared_ptr<const MapCell> &map_cell,
	const boost::shared_ptr<PlayerSession> &session)
{
	map_cell->synchronize_with_client(session);
}
inline void synchronize_map_cell_with_client(const boost::shared_ptr<MapCell> &map_cell,
	const boost::shared_ptr<PlayerSession> &session)
{
	map_cell->synchronize_with_client(session);
}

}

#endif
