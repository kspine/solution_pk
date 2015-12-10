#ifndef EMPERY_CLUSTER_DATA_COMMON_HPP_
#define EMPERY_CLUSTER_DATA_COMMON_HPP_

#include <cstddef>
#include <boost/shared_ptr.hpp>
#include <vector>
#include <poseidon/optional_map.hpp>
#include <poseidon/shared_nts.hpp>
#include "../id_types.hpp"

namespace EmperyCluster {

namespace Data {
	extern std::vector<Poseidon::OptionalMap> sync_load_data(const char *file);
}

}

#endif
