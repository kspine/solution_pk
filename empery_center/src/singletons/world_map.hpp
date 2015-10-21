#ifndef EMPERY_CENTER_SINGLETONS_WORLD_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_WORLD_MAP_HPP_

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/cstdint.hpp>
#include <boost/container/flat_map.hpp>
#include <utility>

namespace EmperyCenter {

class ClusterSession;

struct WorldMap {
	static boost::shared_ptr<ClusterSession> get(boost::int64_t mapX, boost::int64_t mapY);
	static void getAll(boost::container::flat_map<std::pair<boost::int64_t, boost::int64_t>, boost::shared_ptr<ClusterSession>> &ret);
	static void set(boost::int64_t mapX, boost::int64_t mapY, const boost::shared_ptr<ClusterSession> &session);

	static std::pair<boost::int64_t, boost::int64_t> requireMapCoord(const boost::weak_ptr<ClusterSession> &session);
	static std::pair<boost::uint64_t, boost::uint64_t> requireMapSize();

private:
	WorldMap();
};

}

#endif
