#ifndef EMPERY_CENTER_SINGLETONS_LEGION_TASK_BOX_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_LEGION_TASK_BOX_MAP_HPP_

#include <boost/shared_ptr.hpp>
#include "../id_types.hpp"

namespace EmperyCenter {

class LegionTaskBox;

struct LegionTaskBoxMap {
	static boost::shared_ptr<LegionTaskBox> get(LegionUuid legion_uuid);
	static boost::shared_ptr<LegionTaskBox> require(LegionUuid legion_uuid);
	static void unload(LegionUuid legion_uuid);

private:
	LegionTaskBoxMap() = delete;
};

}

#endif
