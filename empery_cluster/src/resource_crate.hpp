#ifndef EMPERY_CLUSTER_RESOURCE_CRATE_HPP_
#define EMPERY_CLUSTER_RESOURCE_CRATE_HPP_

#include <poseidon/virtual_shared_from_this.hpp>
#include <poseidon/fwd.hpp>
#include <boost/container/flat_map.hpp>
#include <deque>
#include "id_types.hpp"
#include "resource_ids.hpp"
#include "coord.hpp"

namespace EmperyCluster {

class ResourceCrate;

class ClusterClient;
class ResourceCrate : public virtual Poseidon::VirtualSharedFromThis {
private:
	const ResourceCrateUuid                                m_resource_crate_uuid;
	const ResourceId                                       m_resource_id;
	const std::uint64_t                                    m_amount_max;
	const std::uint64_t                                    m_created_time;
	const std::uint64_t                                    m_expiry_time;
	std::uint64_t                                          m_amount_remaining;
	Coord                                                  m_coord;
	const boost::weak_ptr<ClusterClient>                   m_cluster;
public:
	ResourceCrate(ResourceCrateUuid resource_crate_uuid, ResourceId resource_id,
		std::uint64_t amount_max, std::uint64_t  created_time,std::uint64_t  expiry_time,std::uint64_t amount_remaining, boost::weak_ptr<ClusterClient> cluster,
		Coord coord);
	~ResourceCrate();
public:
	ResourceCrateUuid get_resource_crate_uuid() const{
		return m_resource_crate_uuid;
	}
	ResourceId get_resource_id() const{
		return m_resource_id;
	}
	std::uint64_t get_amount_max() const{
		return m_amount_max;
	}
	std::uint64_t get_created_time() const{
		return m_created_time;
	}
	std::uint64_t get_expiry_time() const{
		return m_expiry_time;
	}
	std::uint64_t get_amount_remaining() const{
		return m_amount_remaining;
	}
	void set_amount_remaing(std::uint64_t amount_remaing);
	boost::shared_ptr<ClusterClient> get_cluster() const {
		return m_cluster.lock();
	}

	Coord get_coord() const;
	void set_coord(Coord coord);
};

}

#endif
