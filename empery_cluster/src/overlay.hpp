#ifndef EMPERY_CLUSTER_OVERLAY_HPP_
#define EMPERY_CLUSTER_OVERLAY_HPP_

#include <array>
#include <poseidon/virtual_shared_from_this.hpp>
#include "id_types.hpp"
#include "coord.hpp"

namespace EmperyCluster {

class Overlay : public virtual Poseidon::VirtualSharedFromThis {
private:
	std::array<char, 32> m_overlay_group;
	Coord m_coord;

public:
	//
};

}

#endif
