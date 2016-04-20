#ifndef EMPERY_CENTER_RESOURCE_CRATE_HPP_
#define EMPERY_CENTER_RESOURCE_CRATE_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <boost/container/flat_map.hpp>
#include "id_types.hpp"
#include "coord.hpp"

namespace EmperyCenter {

namespace MySql {
	class Center_ResourceCrate;
}

class MapObject;
class PlayerSession;
class ClusterSession;

class ResourceCrate : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
private:
	const boost::shared_ptr<MySql::Center_ResourceCrate> m_obj;

	// 非持久化数据。
	double m_harvest_remainder = 0;
	boost::weak_ptr<MapObject> m_last_harvester;

public:
	ResourceCrate(ResourceCrateUuid resource_crate_uuid, ResourceId resource_id, std::uint64_t amount_max,
		Coord coord, std::uint64_t created_time, std::uint64_t expiry_time);
	explicit ResourceCrate(boost::shared_ptr<MySql::Center_ResourceCrate> obj);
	~ResourceCrate();

public:
	ResourceCrateUuid get_resource_crate_uuid() const;
	ResourceId get_resource_id() const;
	std::uint64_t get_amount_max() const;
	Coord get_coord() const;
	std::uint64_t get_created_time() const;
	std::uint64_t get_expiry_time() const;
	std::uint64_t get_amount_remaining() const;

	bool has_been_deleted() const;
	void delete_from_game() noexcept;

	boost::shared_ptr<MapObject> get_last_harvester() const {
		return m_last_harvester.lock();
	}

	std::uint64_t harvest(const boost::shared_ptr<MapObject> &harvester, double amount_to_harvest, bool saturated);

	bool is_virtually_removed() const;
	void synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const;
	void synchronize_with_cluster(const boost::shared_ptr<ClusterSession> &cluster) const;
};

inline void synchronize_resource_crate_with_player(const boost::shared_ptr<const ResourceCrate> &resource_crate,
	const boost::shared_ptr<PlayerSession> &session)
{
	resource_crate->synchronize_with_player(session);
}
inline void synchronize_resource_crate_with_player(const boost::shared_ptr<ResourceCrate> &resource_crate,
	const boost::shared_ptr<PlayerSession> &session)
{
	resource_crate->synchronize_with_player(session);
}

inline void synchronize_resource_crate_with_cluster(const boost::shared_ptr<const ResourceCrate> &resource_crate,
	const boost::shared_ptr<ClusterSession> &cluster)
{
	resource_crate->synchronize_with_cluster(cluster);
}
inline void synchronize_resource_crate_with_cluster(const boost::shared_ptr<ResourceCrate> &resource_crate,
	const boost::shared_ptr<ClusterSession> &cluster)
{
	resource_crate->synchronize_with_cluster(cluster);
}

}

#endif
