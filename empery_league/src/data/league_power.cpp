#include "../precompiled.hpp"
#include "league_power.hpp"
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>
#include "data_session.hpp"

namespace EmperyLeague
{
    namespace
    {
        using LeaguePowerContainer = boost::container::flat_map<std::uint64_t, Data::LeaguePower>;
        boost::weak_ptr<const LeaguePowerContainer> g_leaguePower_container;
        const char LEAGUE_POWER_FILE[] = "union_power";

        MODULE_RAII_PRIORITY(handles, 900)
        {
            auto csv = Data::sync_load_data(LEAGUE_POWER_FILE);
            const auto leaguePower_container = boost::make_shared<LeaguePowerContainer>();
            while(csv.fetch_row())
            {
                 Data::LeaguePower elem = { };
                 csv.get(elem.leaguePower_level, "power_level");
                 csv.get(elem.power_id, "power_id");
                 csv.get(elem.power_members_max, "people");
                 csv.get(elem.power_array, "power");

                if(!leaguePower_container->emplace(elem.leaguePower_level, std::move(elem)).second)
                {
                    LOG_EMPERY_LEAGUE_ERROR("Duplicate union_power config: power_level = ", elem.leaguePower_level);
                    DEBUG_THROW(Exception, sslit("Duplicate union_power config"));
                }
            }
            g_leaguePower_container = leaguePower_container;
            handles.push(leaguePower_container);
            auto servlet = DataSession::create_servlet(LEAGUE_POWER_FILE, Data::encode_csv_as_json(csv, "power_level"));
            handles.push(std::move(servlet));
        }
    }

    namespace Data
    {

        const boost::shared_ptr<const LeaguePower> LeaguePower::get(std::uint64_t leaguePower_id)
        {
            PROFILE_ME;

            const auto leaguePower_container = g_leaguePower_container.lock();
            if(!leaguePower_container)
             {
                LOG_EMPERY_LEAGUE_WARNING("corps_power config map has not been loaded.");
                return { };
             }

           const auto it = leaguePower_container->find(leaguePower_id);
            if(it == leaguePower_container->end())
            {
                LOG_EMPERY_LEAGUE_WARNING("corps_power config not found: slot = ", leaguePower_id);
                return { };
             }

            return boost::shared_ptr<const LeaguePower>(leaguePower_container, &(it->second));
        }

        bool LeaguePower::is_have_power(std::uint64_t leaguePower_id,unsigned powerid)
        {

            const auto power = get(leaguePower_id);
            if(power)
            {
                unsigned i = 0;
                for(auto it = power->power_array.begin(); it != power->power_array.end(); ++it)
                {
                    if(boost::lexical_cast<unsigned>(power->power_array.at(i).get<double>()) == powerid)
                    {
              //          LOG_EMPERY_LEAGUE_DEBUG("找到对应权限 ========================================= ", powerid);
                        return true;
                    }
                    i++;
                }
            }


     //       LOG_EMPERY_LEAGUE_DEBUG("没找到对应权限 ========================================= ", powerid);

            return false;
        }

        std::uint64_t LeaguePower::get_member_max_num(std::uint64_t leaguePower_id)
        {
            const auto powerinfo = get(leaguePower_id);
            if(powerinfo)
            {
           //     LOG_EMPERY_CENTER_DEBUG("找到对应等级的最大人数 ========================================= ", powerinfo->power_members_max);

                return powerinfo->power_members_max;
            }

            return 0;
        }
    }
}