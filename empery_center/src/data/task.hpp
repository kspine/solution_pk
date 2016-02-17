#ifndef EMPERY_CENTER_DATA_TASK_HPP_
#define EMPERY_CENTER_DATA_TASK_HPP_

#include "common.hpp"
#include <boost/container/flat_map.hpp>
#include <vector>

namespace EmperyCenter {

namespace Data {
	class TaskAbstract {
	public:
		static boost::shared_ptr<const TaskAbstract> get(TaskId task_id);
		static boost::shared_ptr<const TaskAbstract> require(TaskId task_id);

	public:
		TaskId task_id;
		TaskTypeId type;
		boost::container::flat_map<std::uint64_t, std::vector<std::uint64_t>> objective;
		boost::container::flat_map<ItemId, std::uint64_t> reward;
	};

	class TaskPrimary : public TaskAbstract {
	public:
		static boost::shared_ptr<const TaskPrimary> get(TaskId task_id);
		static boost::shared_ptr<const TaskPrimary> require(TaskId task_id);

		static void get_all(std::vector<boost::shared_ptr<const TaskPrimary>> &ret);

	public:
		TaskId previous_id;
	};

	class TaskDaily : public TaskAbstract {
	public:
		static boost::shared_ptr<const TaskDaily> get(TaskId task_id);
		static boost::shared_ptr<const TaskDaily> require(TaskId task_id);

		static void get_all(std::vector<boost::shared_ptr<const TaskDaily>> &ret);

	public:
		unsigned level_limit_min;
		unsigned level_limit_max;
	};
}

}

#endif
