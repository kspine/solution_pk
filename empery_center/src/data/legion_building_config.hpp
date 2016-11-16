#ifndef EMPERY_CENTER_DATA_LEGION_BUILDING_IN_HPP_
#define EMPERY_CENTER_DATA_LEGION_BUILDING_IN_HPP_

#include "common.hpp"
#include <boost/container/flat_map.hpp>

namespace EmperyCenter
{
    namespace Data
    {
        class LegionBuilding
        {
            public:
               static boost::shared_ptr<const LegionBuilding> get(std::uint64_t house_level);
               static boost::shared_ptr<const LegionBuilding> require(std::uint64_t house_level);
               static std::uint64_t get_max_resource(std::uint64_t house_level);
               static std::uint64_t get_open_time(std::uint64_t house_level);
            public:
               std::uint64_t house_level;

               std::uint64_t  levelup_time;
               std::uint64_t  building_id;
               std::uint64_t  open_time;
               std::uint64_t  effect_time;
               std::uint64_t  demolition;
               std::uint64_t  building_combat_attributes;
              
               std::uint64_t max_resource;
               boost::container::flat_map<std::string, std::uint64_t> need_resource;
               boost::container::flat_map<std::string, std::uint64_t> consumption_item;
               boost::container::flat_map<std::string, std::uint64_t> obtain_item;
               boost::container::flat_map<std::string, std::uint64_t> need;
               boost::container::flat_map<std::uint64_t, std::string> damage_buff;

                boost::container::flat_map<std::uint64_t, boost::container::flat_map<std::uint64_t, boost::container::flat_map<std::uint64_t, std::uint64_t> > >  open_effect ;
        };
    }
}
#endif //
