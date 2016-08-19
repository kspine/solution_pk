#include "../precompiled.hpp"
#include "legion_corps_power.hpp"
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>
#include "../data_session.hpp"

namespace EmperyCenter
{
    namespace
    {
        using LegionCorpsPowerContainer = boost::container::flat_map<LegionCorpsPowerId, Data::LegionCorpsPower>;
        boost::weak_ptr<const LegionCorpsPowerContainer> g_legionCorpsPower_container;
        const char CORPS_POWER_FILE[] = "corps_power";

        MODULE_RAII_PRIORITY(handles, 900)
        {
            auto csv = Data::sync_load_data(CORPS_POWER_FILE);
            const auto legionCorpsPower_container = boost::make_shared<LegionCorpsPowerContainer>();
            while(csv.fetch_row())
            {
                 Data::LegionCorpsPower elem = { };
                 csv.get(elem.legionCorpsPower_level, "power_level");
                 csv.get(elem.power_id, "power_id");
                 csv.get(elem.power_members_max, "people");
                 csv.get(elem.power_array, "power");

                if(!legionCorpsPower_container->emplace(elem.legionCorpsPower_level, std::move(elem)).second)
                {
                    LOG_EMPERY_CENTER_ERROR("Duplicate corps_power config: power_level = ", elem.legionCorpsPower_level);
                    DEBUG_THROW(Exception, sslit("Duplicate corps_power config"));
                }
            }
            g_legionCorpsPower_container = legionCorpsPower_container;
            handles.push(legionCorpsPower_container);
            auto servlet = DataSession::create_servlet(CORPS_POWER_FILE, Data::encode_csv_as_json(csv, "power_level"));
            handles.push(std::move(servlet));
        }
    }

    namespace Data
    {

        const boost::shared_ptr<const LegionCorpsPower> LegionCorpsPower::get(LegionCorpsPowerId legionCorpsPower_id)
        {
            PROFILE_ME;

            const auto legionCorpsPower_container = g_legionCorpsPower_container.lock();
            if(!legionCorpsPower_container)
             {
                    LOG_EMPERY_CENTER_WARNING("corps_power config map has not been loaded.");
                    return { };
             }

           const auto it = legionCorpsPower_container->find(legionCorpsPower_id);
            if(it == legionCorpsPower_container->end())
            {
                LOG_EMPERY_CENTER_WARNING("corps_power config not found: slot = ", legionCorpsPower_id);
                return { };
             }

            return boost::shared_ptr<const LegionCorpsPower>(legionCorpsPower_container, &(it->second));
        }

        bool LegionCorpsPower::is_have_power(LegionCorpsPowerId legionCorpsPower_id,unsigned powerid)
        {
            
            const auto power = get(legionCorpsPower_id);
            if(power)
            {
                unsigned i = 0;
                for(auto it = power->power_array.begin(); it != power->power_array.end(); ++it)
                {
                    if(boost::lexical_cast<unsigned>(power->power_array.at(i).get<double>()) == powerid)
                    {
              //          LOG_EMPERY_CENTER_DEBUG("找到对应权限 ========================================= ", powerid);
                        return true;
                    }
                    i++;
                }
            }


            LOG_EMPERY_CENTER_DEBUG("没找到对应权限 ========================================= ", powerid);

            return false;
        }

        std::uint64_t LegionCorpsPower::get_member_max_num(LegionCorpsPowerId legionCorpsPower_id)
        {
            const auto powerinfo = get(legionCorpsPower_id);
            if(powerinfo)
            {
           //     LOG_EMPERY_CENTER_DEBUG("找到对应等级的最大人数 ========================================= ", powerinfo->power_members_max);

                return powerinfo->power_members_max;
            }

            return 0;
        }
    }
}