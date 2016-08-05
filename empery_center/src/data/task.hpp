#ifndef EMPERY_CENTER_DATA_TASK_HPP_
#define EMPERY_CENTER_DATA_TASK_HPP_

#include "common.hpp"
#include <boost/container/flat_map.hpp>
#include <vector>

namespace EmperyCenter {

namespace Data {
	class TaskAbstract {
	public:
		enum CastleCategory {
			CC_PRIMARY      = 1,
			CC_ALL          = 2,
			CC_NON_PRIMARY  = 3,
		};

	public:
		static boost::shared_ptr<const TaskAbstract> get(TaskId task_id);
		static boost::shared_ptr<const TaskAbstract> require(TaskId task_id);

	public:
		TaskId task_id;
		unsigned castle_category;
		TaskTypeId type;
		bool accumulative;
		boost::container::flat_map<std::uint64_t, std::vector<double>> objective;
		boost::container::flat_map<ResourceId, std::uint64_t> rewards;
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
