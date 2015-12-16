#ifndef EMPERY_CLUSTER_OVERLAY_HPP_
#define EMPERY_CLUSTER_OVERLAY_HPP_

#include <array>
#include <poseidon/virtual_shared_from_this.hpp>
#include "id_types.hpp"
#include "coord.hpp"

namespace EmperyCluster {

class Overlay : public virtual Poseidon::VirtualSharedFromThis {
private:
	const Coord m_cluster_coord;
	const std::string m_overlay_group_name;
	const OverlayId m_overlay_id;

	const Coord m_coord;
	const ResourceId m_resource_id;

	boost::uint64_t m_resource_amount;

public:
	Overlay(Coord cluster_coord, std::string overlay_group_name, OverlayId overlay_id,
		Coord coord, ResourceId resource_id, boost::uint64_t resource_amount);
	~Overlay();

public:
	Coord get_cluster_coord() const {
		return m_cluster_coord;
	}
	const std::string &get_overlay_group_name() const {
		return m_overlay_group_name;
	}
	OverlayId get_overlay_id() const {
		return m_overlay_id;
	}

	Coord get_coord() const {
		return m_coord;
	}
	ResourceId get_resource_id() const {
		return m_resource_id;
	}

	boost::uint64_t get_resource_amount() const {
		return m_resource_amount;
	}
};

}

#endif
