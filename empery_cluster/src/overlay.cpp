#include "precompiled.hpp"
#include "overlay.hpp"

namespace EmperyCluster {

Overlay::Overlay(Coord cluster_coord, std::string overlay_group_name, OverlayId overlay_id,
	Coord coord, ResourceId resource_id, boost::uint64_t resource_amount)
	: m_cluster_coord(cluster_coord), m_overlay_group_name(std::move(overlay_group_name)), m_overlay_id(overlay_id)
	, m_coord(coord), m_resource_id(resource_id), m_resource_amount(resource_amount)
{
}
Overlay::~Overlay(){
}

}
