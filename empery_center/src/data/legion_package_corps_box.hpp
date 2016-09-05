#ifndef EMPERY_CENTER_DATA_LEGION_PACKAGE_CORPS_BOX_IN_HPP_
#define EMPERY_CENTER_DATA_LEGION_PACKAGE_CORPS_BOX_IN_HPP_

#include "common.hpp"
#include <boost/container/flat_map.hpp>
#include <vector>

namespace EmperyCenter
{
	namespace Data
	{
		class LegionPackageCorpsBox
		{
		public:
			static boost::shared_ptr<const LegionPackageCorpsBox> get(TaskId task_id);
			static boost::shared_ptr<const LegionPackageCorpsBox> require(TaskId task_id);

		public:
			static bool check_legion_task_package(TaskId task_id, std::uint64_t package_id, std::uint64_t legion_level);
			static bool check_legion_share_package(TaskId task_id, std::uint64_t package_id, std::uint64_t legion_level);
		public:
			TaskId task_id;

			std::uint64_t  task_type;
			boost::container::flat_map<std::string,std::string> task_package_reward_map;
			boost::container::flat_map<std::string,std::string> share_package_reward_map;
		};
	}
}
#endif //