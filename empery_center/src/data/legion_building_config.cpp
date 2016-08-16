#include "../precompiled.hpp"
#include "legion_building_config.hpp"
#include <poseidon/multi_index_map.hpp>
#include <string.h>
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>
#include "../data_session.hpp"

namespace EmperyCenter
{
    namespace
    {
        MULTI_INDEX_MAP(LegionBuildingContainer, Data::LegionBuilding,
            UNIQUE_MEMBER_INDEX(house_level))

        boost::weak_ptr<const LegionBuildingContainer> g_LegionBuilding_container;
	    const char CORPS_FILE[] = "corps_house";

        MODULE_RAII_PRIORITY(handles, 1000)
        {
            auto csv = Data::sync_load_data(CORPS_FILE);
            const auto LegionBuilding_container = boost::make_shared<LegionBuildingContainer>();

            while(csv.fetch_row())
            {
                    Data::LegionBuilding elem = { };

                    csv.get(elem.house_level, "house_level");
                    csv.get(elem.levelup_time, "levelup_time");
                    csv.get(elem.building_id, "building_id");
                    csv.get(elem.open_time, "open_time");
                    csv.get(elem.effect_time, "effect_time");
                    csv.get(elem.demolition, "demolition");
                    csv.get(elem.building_combat_attributes, "building_combat_attributes");

                    Poseidon::JsonObject object;

                    // 消耗资源
                    csv.get(object, "need_resource");
                    elem.need_resource.reserve(object.size());

                    for(auto it = object.begin(); it != object.end(); ++it)
                    {
                        auto collection_name = std::string(it->first.get());
                        const auto count = static_cast<std::uint64_t>(it->second.get<double>());
                        if(!elem.need_resource.emplace(std::move(collection_name), count).second)
                        {
                            LOG_EMPERY_CENTER_ERROR("need_resource map: collection_name = ", collection_name);
                            DEBUG_THROW(Exception, sslit("Duplicate level_up_cost map"));
                        }
                    }

                    // 消耗道具
                    csv.get(object, "consumption_item");
                    elem.consumption_item.reserve(object.size());

                    for(auto it = object.begin(); it != object.end(); ++it)
                    {
                        auto collection_name = std::string(it->first.get());
                        const auto count = static_cast<std::uint64_t>(it->second.get<double>());
                        if(!elem.consumption_item.emplace(std::move(collection_name), count).second)
                        {
                            LOG_EMPERY_CENTER_ERROR("consumption_item map: collection_name = ", collection_name);
                            DEBUG_THROW(Exception, sslit("Duplicate level_up_cost map"));
                        }
                    }

                     // 获得道具
                    csv.get(object, "obtain_item");
                    elem.obtain_item.reserve(object.size());

                    for(auto it = object.begin(); it != object.end(); ++it)
                    {
                        auto collection_name = std::string(it->first.get());
                        const auto count = static_cast<std::uint64_t>(it->second.get<double>());
                        if(!elem.obtain_item.emplace(std::move(collection_name), count).second)
                        {
                            LOG_EMPERY_CENTER_ERROR("obtain_item map: collection_name = ", collection_name);
                            DEBUG_THROW(Exception, sslit("Duplicate level_up_cost map"));
                        }
                    }

                    // 升级条件
                    csv.get(object, "need");
                    elem.need.reserve(object.size());

                    for(auto it = object.begin(); it != object.end(); ++it)
                    {
                        auto collection_name = std::string(it->first.get());
                        const auto count = static_cast<std::uint64_t>(it->second.get<double>());
                        if(!elem.need.emplace(std::move(collection_name), count).second)
                        {
                            LOG_EMPERY_CENTER_ERROR("need map: collection_name = ", collection_name);
                            DEBUG_THROW(Exception, sslit("Duplicate level_up_cost map"));
                        }
                    }

                    // 战损属性
                    csv.get(object, "damage_buff");
                    elem.damage_buff.reserve(object.size());
                    for(auto it = object.begin(); it != object.end(); ++it)
                    {
                        auto collection_name = std::string(it->first.get());
                        const auto count = static_cast<std::uint64_t>(it->second.get<double>());
                        if(!elem.damage_buff.emplace(count, std::move(collection_name)).second)
                        {
                            LOG_EMPERY_CENTER_ERROR("damage_buff map: collection_name = ", collection_name);
                            DEBUG_THROW(Exception, sslit("Duplicate level_up_cost map"));
                        }
                    }

                    if(!LegionBuilding_container->insert(std::move(elem)).second)
                    {
                        LOG_EMPERY_CENTER_ERROR("Duplicate LegionBuilding: house_level = ", elem.house_level);
                        DEBUG_THROW(Exception, sslit("Duplicate LegionBuilding"));
                    }
            }


		   g_LegionBuilding_container = LegionBuilding_container;
		   handles.push(LegionBuilding_container);
		   auto servlet = DataSession::create_servlet(CORPS_FILE, Data::encode_csv_as_json(csv, "house_level"));
		   handles.push(std::move(servlet));

        }
   }

    namespace Data
    {
        boost::shared_ptr<const LegionBuilding> LegionBuilding::get(std::uint64_t house_level)
        {
		   PROFILE_ME;

		   const auto LegionBuilding_container = g_LegionBuilding_container.lock();
		   if(!LegionBuilding_container)
           {
			 LOG_EMPERY_CENTER_WARNING("LegionBuildingContainer has not been loaded.");
			 return { };
		   }

		   const auto it = LegionBuilding_container->find<0>(house_level);
		   if(it == LegionBuilding_container->end<0>())
           {
			 LOG_EMPERY_CENTER_TRACE("Corps not found: house_level = ", house_level);
			 return { };
		   }
		   return boost::shared_ptr<const LegionBuilding>(LegionBuilding_container, &*it);
        }

	    boost::shared_ptr<const LegionBuilding> LegionBuilding::require(std::uint64_t house_level)
        {
		   PROFILE_ME;
		   auto ret = get(house_level);
		   if(!ret)
           {
			 LOG_EMPERY_CENTER_WARNING("Corps not found: house_level = ", house_level);
			 DEBUG_THROW(Exception, sslit("Corps not found"));
		   }
		   return ret;
        }
	}
}