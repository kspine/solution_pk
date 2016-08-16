#include "../precompiled.hpp"
#include "legion_corps_level.hpp"
#include <poseidon/multi_index_map.hpp>
#include <string.h>
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>
#include "../data_session.hpp"

namespace EmperyCenter
{
    namespace
    {
        MULTI_INDEX_MAP(LegionCorpsLevelContainer, Data::LegionCorpsLevel,
            UNIQUE_MEMBER_INDEX(legionCorpsLevel_id))

        boost::weak_ptr<const LegionCorpsLevelContainer> g_legionCorpsLevel_container;
	    const char CORPS_FILE[] = "corps";

        MODULE_RAII_PRIORITY(handles, 1000)
        {
            auto csv = Data::sync_load_data(CORPS_FILE);
            const auto legionCorpsLevel_container = boost::make_shared<LegionCorpsLevelContainer>();
            while(csv.fetch_row())
            {
                    Data::LegionCorpsLevel elem = { };

                    csv.get(elem.legionCorpsLevel_id, "corps_level");
                    csv.get(elem.level_up_time, "level_up_time");
                    csv.get(elem.legion_member_max, "effect_max");

                    Poseidon::JsonObject object;

                        //军团升级消耗
                        csv.get(object, "need_resource");
                        elem.level_up_cost_map.reserve(object.size());

                        for(auto it = object.begin(); it != object.end(); ++it)
                        {
                            auto collection_name = std::string(it->first.get());
                            const auto count = static_cast<std::uint64_t>(it->second.get<double>());
                            if(!elem.level_up_cost_map.emplace(std::move(collection_name), count).second)
                            {
                                LOG_EMPERY_CENTER_ERROR("Duplicate level_up_cost map: collection_name = ", collection_name);
                                DEBUG_THROW(Exception, sslit("Duplicate level_up_cost map"));
                            }
                        }

                        //军团升级限制
                        csv.get(object, "need");
                        elem.level_up_limit_map.reserve(object.size());
                        for(auto it = object.begin(); it != object.end(); ++it)
                        {
                            auto collection_name = std::string(it->first.get());
                            const auto count = static_cast<std::uint64_t>(it->second.get<double>());
                            if(!elem.level_up_limit_map.emplace(std::move(collection_name), count).second)
                            {
                                LOG_EMPERY_CENTER_ERROR("Duplicate levlevel_up_limit map: collection_name = ", collection_name);
                                DEBUG_THROW(Exception, sslit("Duplicate level_up_limit map"));
                            }
                        }

                    if(!legionCorpsLevel_container->insert(std::move(elem)).second)
                    {
                        LOG_EMPERY_CENTER_ERROR("Duplicate legionCorpsLevel: legionCorpsLevel_Id = ", elem.legionCorpsLevel_id);
                        DEBUG_THROW(Exception, sslit("Duplicate legionCorpsLevel"));
                    }

                LOG_EMPERY_CENTER_ERROR("Load  corps.csv : success!!!");

            }

		   g_legionCorpsLevel_container = legionCorpsLevel_container;
		   handles.push(legionCorpsLevel_container);
		   auto servlet = DataSession::create_servlet(CORPS_FILE, Data::encode_csv_as_json(csv, "corps_level"));
		   handles.push(std::move(servlet));
        }
   }

    namespace Data
    {
        boost::shared_ptr<const LegionCorpsLevel> LegionCorpsLevel::get(LegionCorpsLevelId legionCorpsLevel_id)
        {
		   PROFILE_ME;

		   const auto legionCorpsLevel_container = g_legionCorpsLevel_container.lock();
		   if(!legionCorpsLevel_container)
           {
			 LOG_EMPERY_CENTER_WARNING("LegionCorpsLevelContainer has not been loaded.");
			 return { };
		   }

		   const auto it = legionCorpsLevel_container->find<0>(legionCorpsLevel_id);
		   if(it == legionCorpsLevel_container->end<0>())
           {
			 LOG_EMPERY_CENTER_TRACE("Corps not found: corps_level = ", legionCorpsLevel_id);
			 return { };
		   }
		   return boost::shared_ptr<const LegionCorpsLevel>(legionCorpsLevel_container, &*it);
        }

	    boost::shared_ptr<const LegionCorpsLevel> LegionCorpsLevel::require(LegionCorpsLevelId legionCorpsLevel_id)
        {
		   PROFILE_ME;
		   auto ret = get(legionCorpsLevel_id);
		   if(!ret)
           {
			 LOG_EMPERY_CENTER_WARNING("Corps not found: corps_level = ", legionCorpsLevel_id);
			 DEBUG_THROW(Exception, sslit("Corps not found"));
		   }
		   return ret;
        }
	}
}