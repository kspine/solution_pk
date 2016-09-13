#ifndef EMPERY_CENTER_DATA_LEGION_CORPS_POWER_IN_HPP_
#define EMPERY_CENTER_DATA_LEGION_CORPS_POWER_IN_HPP_

#include "common.hpp"
#include <boost/container/flat_map.hpp>
#include <string>
#include <poseidon/json.hpp>

namespace EmperyCenter
{
    namespace Data
    {
        class LegionCorpsPower
        {

            public:

                static const boost::shared_ptr<const LegionCorpsPower> get(LegionCorpsPowerId legionCorpsPower_id);

                static std::uint64_t get_member_max_num(LegionCorpsPowerId legionCorpsPower_id);

                static bool is_have_power(LegionCorpsPowerId legionCorpsPower_id,unsigned powerid);

            public:
               LegionCorpsPowerId legionCorpsPower_level;
               std::uint64_t  power_id;
               std::uint64_t  power_members_max;  
               Poseidon::JsonArray power_array;
        };
    }
}
#endif //
