#include "precompiled.hpp"
#include "resource_crate.hpp"
#include "mmain.hpp"
#include <poseidon/singletons/timer_daemon.hpp>
#include <poseidon/cbpp/status_codes.hpp>
#include <poseidon/json.hpp>
#include "cluster_client.hpp"
#include "singletons/world_map.hpp"
#include "checked_arithmetic.hpp"
#include "map_utilities.hpp"
#include "../../empery_center/src/msg/sc_map.hpp"
#include "../../empery_center/src/msg/ks_map.hpp"
#include "../../empery_center/src/msg/err_map.hpp"
#include "../../empery_center/src/attribute_ids.hpp"



namespace EmperyCluster {

namespace Msg = ::EmperyCenter::Msg;


ResourceCrate::ResourceCrate(ResourceCrateUuid resource_crate_uuid, ResourceId resource_id,
		std::uint64_t amount_max, std::uint64_t  created_time,std::uint64_t  expiry_time,std::uint64_t amount_remaining, boost::weak_ptr<ClusterClient> cluster,
		Coord coord)
	: m_resource_crate_uuid(resource_crate_uuid), m_resource_id(resource_id)
	, m_amount_max(amount_max), m_created_time(created_time),m_expiry_time(expiry_time),m_amount_remaining(amount_remaining),m_coord(coord),m_cluster(std::move(cluster))
{
}
ResourceCrate::~ResourceCrate(){
}

void ResourceCrate::set_amount_remaing(std::uint64_t amount_remaing){
	m_amount_remaining = amount_remaing;
}

Coord ResourceCrate::get_coord() const {
	return m_coord;
}
void ResourceCrate::set_coord(Coord coord){
	PROFILE_ME;

	if(get_coord() == coord){
		return;
	}
	m_coord = coord;

	WorldMap::update_resource_crate(virtual_shared_from_this<ResourceCrate>(), false);
}
}
