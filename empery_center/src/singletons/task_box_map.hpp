#ifndef EMPERY_CENTER_SINGLETONS_TASK_BOX_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_TASK_BOX_MAP_HPP_

#include <boost/shared_ptr.hpp>
#include "../id_types.hpp"

namespace EmperyCenter {

class TaskBox;

struct TaskBoxMap {
	static boost::shared_ptr<TaskBox> get(AccountUuid account_uuid);
	static boost::shared_ptr<TaskBox> require(AccountUuid account_uuid);

private:
	TaskBoxMap() = delete;
};

}

#endif
