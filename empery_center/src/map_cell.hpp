#ifndef EMPERY_CENTER_MAP_CELL_HPP_
#define EMPERY_CENTER_MAP_CELL_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <boost/container/flat_map.hpp>
#include "id_types.hpp"
#include "coord.hpp"

namespace EmperyCenter {

namespace MySql {
	class Center_MapCell;
	class Center_MapCellAttribute;
}

class PlayerSession;
class ClusterSession;

class MapCell : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
private:
	boost::shared_ptr<MySql::Center_MapCell> m_obj;
	boost::container::flat_map<AttributeId,
		boost::shared_ptr<MySql::Center_MapCellAttribute>> m_attributes;

	// 非持久化数据。
	double m_production_remainder = 0;
	double m_production_rate = 0;
	std::uint64_t m_capacity = 0;

public:
	explicit MapCell(Coord coord);
	MapCell(boost::shared_ptr<MySql::Center_MapCell> obj,
		const std::vector<boost::shared_ptr<MySql::Center_MapCellAttribute>> &attributes);
	~MapCell();

public:
	virtual void pump_status();

	Coord get_coord() const;

	MapObjectUuid get_parent_object_uuid() const;
	AccountUuid get_owner_uuid() const;

	bool is_acceleration_card_applied() const;
	void set_acceleration_card_applied(bool value);

	ItemId get_ticket_item_id() const;
	ResourceId get_production_resource_id() const;
	std::uint64_t get_last_production_time() const;
	std::uint64_t get_resource_amount() const;

	double get_production_rate() const {
		return m_production_rate;
	}
	std::uint64_t get_capacity() const {
		return m_capacity;
	}

	void pump_production();

	void set_parent_object(MapObjectUuid parent_object_uuid, ResourceId production_resource_id, ItemId ticket_item_id);
	void set_ticket_item_id(ItemId ticket_item_id);

	std::uint64_t harvest(bool saturated);

	std::int64_t get_attribute(AttributeId attribute_id) const;
	void get_attributes(boost::container::flat_map<AttributeId, std::int64_t> &ret) const;
	void set_attributes(const boost::container::flat_map<AttributeId, std::int64_t> &modifiers);

	bool is_virtually_removed() const;
	void synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const;
	void synchronize_with_cluster(const boost::shared_ptr<ClusterSession> &cluster) const;
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
inline void synchronize_map_cell_with_cluster(const boost::shared_ptr<const MapCell> &map_cell,
	const boost::shared_ptr<ClusterSession> &cluster)
{
	map_cell->synchronize_with_cluster(cluster);
}
inline void synchronize_map_cell_with_cluster(const boost::shared_ptr<MapCell> &map_cell,
	const boost::shared_ptr<ClusterSession> &cluster)
{
	map_cell->synchronize_with_cluster(cluster);
}

}

#endif
