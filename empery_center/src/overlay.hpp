#ifndef EMPERY_CENTER_OVERLAY_HPP_
#define EMPERY_CENTER_OVERLAY_HPP_

#include <poseidon/virtual_shared_from_this.hpp>
#include "id_types.hpp"
#include "coord.hpp"

namespace EmperyCenter {

namespace MySql {
	class Center_Overlay;
}

class Castle;
class PlayerSession;
class ClusterSession;

class Overlay : public virtual Poseidon::VirtualSharedFromThis {
private:
	boost::shared_ptr<MySql::Center_Overlay> m_obj;

public:
	Overlay(Coord cluster_coord, std::string overlay_group_name, OverlayId overlay_id);
	explicit Overlay(boost::shared_ptr<MySql::Center_Overlay> obj);
	~Overlay();

public:
	Coord get_cluster_coord() const;
	const std::string &get_overlay_group_name() const;
	OverlayId get_overlay_id() const;

	ResourceId get_resource_id() const;
	std::uint64_t get_resource_amount() const;

	std::uint64_t harvest(const boost::shared_ptr<Castle> &castle, std::uint64_t max_amount, bool saturated);

	bool is_virtually_removed() const;
	void synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const;
	void synchronize_with_cluster(const boost::shared_ptr<ClusterSession> &cluster) const;
};

inline void synchronize_overlay_with_player(const boost::shared_ptr<const Overlay> &overlay,
	const boost::shared_ptr<PlayerSession> &session)
{
	overlay->synchronize_with_player(session);
}
inline void synchronize_overlay_with_player(const boost::shared_ptr<Overlay> &overlay,
	const boost::shared_ptr<PlayerSession> &session)
{
	overlay->synchronize_with_player(session);
}
inline void synchronize_overlay_with_cluster(const boost::shared_ptr<const Overlay> &overlay,
	const boost::shared_ptr<ClusterSession> &cluster)
{
	overlay->synchronize_with_cluster(cluster);
}
inline void synchronize_overlay_with_cluster(const boost::shared_ptr<Overlay> &overlay,
	const boost::shared_ptr<ClusterSession> &cluster)
{
	overlay->synchronize_with_cluster(cluster);
}

}

#endif
