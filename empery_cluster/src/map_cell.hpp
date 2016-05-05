#ifndef EMPERY_CLUSTER_MAP_CELL_HPP_
#define EMPERY_CLUSTER_MAP_CELL_HPP_

#include <poseidon/virtual_shared_from_this.hpp>
#include <poseidon/fwd.hpp>
#include <boost/container/flat_map.hpp>
#include "id_types.hpp"
#include "coord.hpp"

namespace EmperyCluster {

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
	boost::container::flat_map<AttributeId, std::int64_t> m_attributes;
	boost::container::flat_map<BuffId, BuffInfo>          m_buffs;

public:
	MapCell(Coord coord, MapObjectUuid parent_object_uuid, AccountUuid owner_uuid,
		boost::container::flat_map<AttributeId,std::int64_t> attributes,boost::container::flat_map<BuffId, BuffInfo> buffs);
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

	std::int64_t get_attribute(AttributeId map_object_attr_id) const;
	void get_attributes(boost::container::flat_map<AttributeId, std::int64_t> &ret) const;
	BuffInfo get_buff(BuffId buff_id) const;
	void get_buffs(std::vector<BuffInfo> &ret) const;
	void set_buff(BuffId buff_id, std::uint64_t time_begin, std::uint64_t duration);
	void clear_buff(BuffId buff_id) noexcept;
};

}

#endif
