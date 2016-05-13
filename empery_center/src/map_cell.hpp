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
	class Center_MapCellBuff;
}

class Castle;
class PlayerSession;
class ClusterSession;

class MapCell : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
public:
	struct BuffInfo {
		BuffId buff_id;
		std::uint64_t duration;
		std::uint64_t time_begin;
		std::uint64_t time_end;
	};

private:
	const boost::shared_ptr<MySql::Center_MapCell> m_obj;

	boost::container::flat_map<AttributeId,
		boost::shared_ptr<MySql::Center_MapCellAttribute>> m_attributes;
	boost::container::flat_map<BuffId,
		boost::shared_ptr<MySql::Center_MapCellBuff>> m_buffs;

	// 非持久化数据。
	double m_production_remainder = 0;
	double m_production_rate = 0;
	double m_capacity = 0;

public:
	explicit MapCell(Coord coord);
	MapCell(boost::shared_ptr<MySql::Center_MapCell> obj,
		const std::vector<boost::shared_ptr<MySql::Center_MapCellAttribute>> &attributes,
		const std::vector<boost::shared_ptr<MySql::Center_MapCellBuff>> &buffs);
	~MapCell();

public:
	virtual bool should_auto_update() const;
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
	double get_capacity() const {
		return m_capacity;
	}

	void pump_production();

	void set_parent_object(MapObjectUuid parent_object_uuid, ResourceId production_resource_id, ItemId ticket_item_id);
	void set_ticket_item_id(ItemId ticket_item_id);

	std::uint64_t harvest(const boost::shared_ptr<Castle> &castle, bool saturated);

	std::int64_t get_attribute(AttributeId attribute_id) const;
	void get_attributes(boost::container::flat_map<AttributeId, std::int64_t> &ret) const;
	void set_attributes(const boost::container::flat_map<AttributeId, std::int64_t> &modifiers);

	BuffInfo get_buff(BuffId buff_id) const;
	bool is_buff_in_effect(BuffId buff_id) const;
	void get_buffs(std::vector<BuffInfo> &ret) const;
	void set_buff(BuffId buff_id, std::uint64_t time_begin, std::uint64_t duration);
	void accumulate_buff(BuffId buff_id, std::uint64_t delta_duration);
	void clear_buff(BuffId buff_id) noexcept;

	MapObjectUuid get_occupier_object_uuid() const;
	AccountUuid get_occupier_owner_uuid() const;
	void set_occupier_object_uuid(MapObjectUuid occupier_object_uuid);

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
