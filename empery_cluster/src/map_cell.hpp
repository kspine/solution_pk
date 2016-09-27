#ifndef EMPERY_CLUSTER_MAP_CELL_HPP_
#define EMPERY_CLUSTER_MAP_CELL_HPP_

#include <poseidon/virtual_shared_from_this.hpp>
#include <poseidon/fwd.hpp>
#include <boost/container/flat_map.hpp>
#include "id_types.hpp"
#include "coord.hpp"

namespace EmperyCluster {
class MapObject;
class MapCell : public virtual Poseidon::VirtualSharedFromThis {
public:
	struct BuffInfo {
		BuffId buff_id;
		std::uint64_t duration;
		std::uint64_t time_begin;
		std::uint64_t time_end;
	};
private:
	const Coord m_coord;

	MapObjectUuid m_parent_object_uuid;
	AccountUuid m_owner_uuid;
	LegionUuid m_legion_uuid;
	bool m_acceleration_card_applied;
	ItemId m_ticket_item_id;
	ResourceId m_production_resource_id;
	std::uint64_t m_resource_amount;
	boost::container::flat_map<AttributeId, std::int64_t> m_attributes;
	boost::container::flat_map<BuffId, BuffInfo>          m_buffs;
	MapObjectUuid m_occupier_object_uuid;
	AccountUuid m_occupier_owner_uuid;

public:
	MapCell(Coord coord, MapObjectUuid parent_object_uuid, AccountUuid owner_uuid,LegionUuid legion_uuid,bool acceleration_card_applied,ItemId ticket_item_id,ResourceId production_resource_id,std::uint64_t resource_amount,
		boost::container::flat_map<AttributeId,std::int64_t> attributes,boost::container::flat_map<BuffId, BuffInfo> buffs,MapObjectUuid occupier_object_uuid,AccountUuid occupier_owner_uuid);
	~MapCell();

public:
	Coord get_coord() const {
		return m_coord;
	}

	MapObjectUuid get_parent_object_uuid() const {
		return m_parent_object_uuid;
	}
	AccountUuid get_owner_uuid() const {
		return m_owner_uuid;
	}
	LegionUuid get_legion_uuid() const {
		return m_legion_uuid;
	}
	void on_attack(boost::shared_ptr<MapObject> attacker);
	bool is_in_group_view_scope(boost::shared_ptr<MapObject> attacker);
	bool is_acceleration_card_applied() const;
	ItemId get_ticket_item_id() const;
	ResourceId get_production_resource_id() const;
	std::uint64_t get_resource_amount() const;

	std::int64_t get_attribute(AttributeId map_object_attr_id) const;
	void get_attributes(boost::container::flat_map<AttributeId, std::int64_t> &ret) const;
	BuffInfo get_buff(BuffId buff_id) const;
	bool is_buff_in_effect(BuffId buff_id) const;
	void get_buffs(std::vector<BuffInfo> &ret) const;
	void set_buff(BuffId buff_id, std::uint64_t time_begin, std::uint64_t duration);
	void clear_buff(BuffId buff_id) noexcept;
	bool is_in_castle_protect();
	bool is_protectable() const;
	bool is_in_noviciate_protect();

	MapObjectUuid get_occupier_object_uuid() const;
	AccountUuid get_occupier_owner_uuid() const;
};

}

#endif
