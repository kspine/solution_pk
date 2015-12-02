#ifndef EMPERY_CLUSTER_MAP_CELL_HPP_
#define EMPERY_CLUSTER_MAP_CELL_HPP_

#include "abstract_data_object.hpp"
#include <poseidon/fwd.hpp>
#include <boost/container/flat_map.hpp>
#include "id_types.hpp"
#include "coord.hpp"

namespace EmperyCluster {

class MapCell : public virtual AbstractDataObject {
private:
	const Coord m_coord;

	AccountUuid m_owner_uuid;
	boost::container::flat_map<AttributeId, boost::int64_t> m_attributes;

public:
	MapCell(Coord coord,
		AccountUuid owner_uuid, boost::container::flat_map<AttributeId, boost::int64_t> attributes);
	~MapCell();

public:
	void pump_status() override;

	Coord get_coord() const {
		return m_coord;
	}

	AccountUuid get_owner_uuid() const {
		return m_owner_uuid;
	}

	boost::int64_t get_attribute(AttributeId map_object_attr_id) const;
	void get_attributes(boost::container::flat_map<AttributeId, boost::int64_t> &ret) const;
	// void set_attributes(const boost::container::flat_map<AttributeId, boost::int64_t> &modifiers);
};

}

#endif
