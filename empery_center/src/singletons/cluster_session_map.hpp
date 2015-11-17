#ifndef EMPERY_CENTER_SINGLETONS_CLUSTER_SESSION_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_CLUSTER_SESSION_MAP_HPP_

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/cstdint.hpp>
#include <boost/container/flat_map.hpp>
#include "../coord.hpp"
#include "../rectangle.hpp"

namespace EmperyCenter {

class ClusterSession;

struct ClusterSessionMap {
	static boost::shared_ptr<ClusterSession> get(Coord server_coord);
	static void get_all(boost::container::flat_map<Coord, boost::shared_ptr<ClusterSession>> &ret);
	static void set(Coord server_coord, const boost::shared_ptr<ClusterSession> &session);

	static Coord require_server_coord(const boost::weak_ptr<ClusterSession> &session);

	static Coord get_server_coord_from_map_coord(Coord coord);
	static Rectangle get_server_map_range(Coord server_coord);

private:
	ClusterSessionMap() = delete;
};

}

#endif
