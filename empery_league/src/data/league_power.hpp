#ifndef EMPERY_LEAGUE_DATA_LEAGUE_POWER_IN_HPP_
#define EMPERY_LEAGUE_DATA_LEAGUE_POWER_IN_HPP_

#include "common.hpp"
#include <boost/container/flat_map.hpp>
#include <string>
#include <poseidon/json.hpp>

namespace EmperyLeague
{
    namespace Data
    {
        class LeaguePower
        {

            public:

                static const boost::shared_ptr<const LeaguePower> get(std::uint64_t leaguePower_id);

                static std::uint64_t get_member_max_num(std::uint64_t leaguePower_id);

                static bool is_have_power(std::uint64_t leaguePower_id,unsigned powerid);

            public:
               std::uint64_t leaguePower_level;
               std::uint64_t  power_id;
               std::uint64_t  power_members_max;
               Poseidon::JsonArray power_array;
        };
    }
}
#endif //
