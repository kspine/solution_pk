#ifndef EMPERY_CENTER_DATA_LEGION_TASK_CONTRIBUTION_IN_HPP_
#define EMPERY_CENTER_DATA_LEGION_TASK_CONTRIBUTION_IN_HPP_
#include "common.hpp"
#include <boost/container/flat_map.hpp>

namespace EmperyCenter
{
	namespace Data
	{
		class LegionTaskContribution
		{
		public:
			static boost::shared_ptr<const LegionTaskContribution> get(LegionTaskContributionId contribution_id);
			static boost::shared_ptr<const LegionTaskContribution> require(LegionTaskContributionId contribution_id);

		public:
			LegionTaskContributionId contribution_id;
			std::uint64_t contribution_key;
			std::uint64_t contribution_value;
		};
	}
}

#endif //