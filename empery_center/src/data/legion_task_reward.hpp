#ifndef EMPERY_CENTER_DATA_LEGION_TASK_REWARD_IN_HPP_
#define EMPERY_CENTER_DATA_LEGION_TASK_REWARD_IN_HPP_
#include "common.hpp"
#include <boost/container/flat_map.hpp>

namespace EmperyCenter
{
	namespace Data
	{
		class LegionTaskReward
		{
		public:
			static boost::shared_ptr<const LegionTaskReward> get(TaskId task_id);
			static boost::shared_ptr<const LegionTaskReward> require(TaskId task_id);

		public:
			TaskId task_id;

			std::uint64_t  task_stage_1;
			std::uint64_t  task_stage_2;
			std::uint64_t  task_stage_3;
			std::uint64_t  task_stage_4;

			boost::container::flat_map<std::string, std::uint64_t> task_package_reward_1_map;
			boost::container::flat_map<std::string, std::uint64_t> task_package_reward_2_map;
			boost::container::flat_map<std::string, std::uint64_t> task_package_reward_3_map;
			boost::container::flat_map<std::string, std::uint64_t> task_package_reward_4_map;
		};
	}
}

#endif //