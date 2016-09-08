#ifndef EMPERY_CENTER_SINGLETONS_LEGION_TASK_CONTRIBUTION_BOX_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_LEGION_TASK_CONTRIBUTION_BOX_MAP_HPP_

#include <boost/shared_ptr.hpp>
#include "../id_types.hpp"

namespace EmperyCenter {

class LegionTaskContributionBox;

struct LegionTaskContributionBoxMap {
	static boost::shared_ptr<LegionTaskContributionBox> get(LegionUuid legion_uuid);
	static boost::shared_ptr<LegionTaskContributionBox> require(LegionUuid legion_uuid);
	static void unload(LegionUuid legion_uuid);

private:
	LegionTaskContributionBoxMap() = delete;
};

}

#endif
