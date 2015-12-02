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

class Castle;
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

	void synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const;

	Coord get_coord() const;

	MapObjectUuid get_parent_object_uuid() const;
	AccountUuid get_owner_uuid() const;

	bool is_acceleration_card_applied() const;
	void set_acceleration_card_applied(bool value);

	ItemId get_ticket_item_id() const;
	ResourceId get_production_resource_id() const;
	boost::uint64_t get_last_production_time() const;
	boost::uint64_t get_resource_amount() const;

	void set_owner(MapObjectUuid parent_object_uuid, ResourceId production_resource_id, ItemId ticket_item_id);
	void set_ticket_item_id(ItemId ticket_item_id);

	void harvest_resource(const boost::shared_ptr<Castle> &castle, boost::uint64_t max_amount = UINT64_MAX);

	boost::int64_t get_attribute(AttributeId attribute_id) const;
	void get_attributes(boost::container::flat_map<AttributeId, boost::int64_t> &ret) const;
	void set_attributes(const boost::container::flat_map<AttributeId, boost::int64_t> &modifiers);
};

inline void synchronize_map_cell_with_player(const boost::shared_ptr<const MapCell> &map_cell,
	const boost::shared_ptr<PlayerSession> &session)
{
	map_cell->synchronize_with_player(session);
}
inline void synchronize_map_cell_with_player(const boost::shared_ptr<MapCell> &map_cell,
	const boost::shared_ptr<PlayerSession> &session)
{
	map_cell->synchronize_with_player(session);
}

}

#endif
