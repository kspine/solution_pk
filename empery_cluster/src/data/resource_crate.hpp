#ifndef EMPERY_CENTER_DATA_RESOURCE_CRATE_HPP_
#define EMPERY_CENTER_DATA_RESOURCE_CRATE_HPP_

#include "common.hpp"
#include <boost/container/flat_map.hpp>

namespace EmperyCluster {

namespace Data {
	class ResourceCrate {
	public:
		static boost::shared_ptr<const ResourceCrate> get(ResourceId resource_id,std::uint64_t amount);
	public:
		std::pair<ResourceId,std::uint64_t> resource_amount;
		double                       defence;
		double                       health;
		unsigned                     defence_type;
	};
}

}

#endif
