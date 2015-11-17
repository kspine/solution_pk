#ifndef EMPERY_CENTER_SINGLETONS_CLUSTER_SESSION_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_CLUSTER_SESSION_MAP_HPP_

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/cstdint.hpp>
#include <boost/container/flat_map.hpp>
#include "../coord.hpp"

namespace EmperyCenter {

class ClusterSession;

struct ClusterSessionMap {
	static boost::shared_ptr<ClusterSession> get(Coord server_coord);
	static void get_all(boost::container::flat_map<Coord, boost::shared_ptr<ClusterSession>> &ret);
	static void set(Coord server_coord, const boost::shared_ptr<ClusterSession> &session);

	static Coord require_map_coord(const boost::weak_ptr<ClusterSession> &session);

	static boost::uint64_t get_map_width() noexcept;
	static boost::uint64_t get_map_height() noexcept;

private:
	ClusterSessionMap() = delete;
};

}

#endif
