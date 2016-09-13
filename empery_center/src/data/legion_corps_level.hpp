#ifndef EMPERY_CENTER_DATA_LEGION_CORPS_LEVEL_IN_HPP_
#define EMPERY_CENTER_DATA_LEGION_CORPS_LEVEL_IN_HPP_

#include "common.hpp"
#include <boost/container/flat_map.hpp>

namespace EmperyCenter
{
    namespace Data
    {
        class LegionCorpsLevel
        {
            public:
               static boost::shared_ptr<const LegionCorpsLevel> get(LegionCorpsLevelId legionCorpsLevel_id);
               static boost::shared_ptr<const LegionCorpsLevel> require(LegionCorpsLevelId legionCorpsLevel_id);
            public:
               LegionCorpsLevelId legionCorpsLevel_id;

               std::uint64_t  level_up_time;
               std::uint64_t  legion_member_max;
               boost::container::flat_map<std::string, std::uint64_t> level_up_cost_map; 
               boost::container::flat_map<std::string, std::uint64_t> level_up_limit_map; 
        };
    }
}
#endif //
