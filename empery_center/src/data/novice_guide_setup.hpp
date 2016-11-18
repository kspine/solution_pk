#ifndef EMPERY_CENTER_DATA_NOVICE_GUIDE_SETUP_IN_HPP_
#define EMPERY_CENTER_DATA_NOVICE_GUIDE_SETUP_IN_HPP_

#include "common.hpp"
#include <boost/container/flat_map.hpp>

namespace EmperyCenter
{
	namespace Data
	{
		class  NoviceGuideSetup
		{
		  public:
				static boost::shared_ptr<const NoviceGuideSetup> get(std::uint64_t step_id);
				static boost::shared_ptr<const NoviceGuideSetup> require(std::uint64_t step_id);

		  public:
				std::uint64_t step_id;

                boost::container::flat_map<ResourceId,std::uint64_t> resource_rewards;
                boost::container::flat_map<MapObjectTypeId,std::uint64_t> arm_rewards;
                boost::container::flat_map<ItemId, std::uint64_t> item_rewards;


	    };
   }
}
#endif //