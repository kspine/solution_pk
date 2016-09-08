#ifndef EMPERY_CENTER_SINGLETONS_LEGION_TASK_REWARD_BOX_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_LEGION_TASK_REWARD_BOX_MAP_HPP_

#include <boost/shared_ptr.hpp>
#include "../id_types.hpp"

namespace EmperyCenter {

class LegionTaskRewardBox;

struct LegionTaskRewardBoxMap {
	static boost::shared_ptr<LegionTaskRewardBox> get(AccountUuid account_uuid);
	static boost::shared_ptr<LegionTaskRewardBox> require(AccountUuid account_uuid);
	static void unload(AccountUuid account_uuid);

private:
	LegionTaskRewardBoxMap() = delete;
};

}

#endif
