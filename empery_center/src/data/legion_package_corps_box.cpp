
#include "../precompiled.hpp"
#include "legion_package_corps_box.hpp"
#include <poseidon/multi_index_map.hpp>
#include <string.h>
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>
#include "../data_session.hpp"

namespace EmperyCenter
{
    namespace
    {
        MULTI_INDEX_MAP(LegionPackageCorpsBoxContainer, Data::LegionPackageCorpsBox,
            UNIQUE_MEMBER_INDEX(task_id))

        boost::weak_ptr<const LegionPackageCorpsBoxContainer> g_legionPackageCorpsBox_container;
	    const char CORPS_BOX_FILE[] = "corps_box";

        MODULE_RAII_PRIORITY(handles, 1000)
        {
            auto csv = Data::sync_load_data(CORPS_BOX_FILE);
            const auto legionPackageCorpsBox_container = boost::make_shared<LegionPackageCorpsBoxContainer>();
            while(csv.fetch_row())
            {
                Data::LegionPackageCorpsBox elem = { };

                csv.get(elem.task_id, "task_id");

                Poseidon::JsonObject object;

                //军团礼包任务奖励
                csv.get(object, "reward1_id");
                elem.task_package_reward_map.reserve(object.size());

                for(auto it = object.begin(); it != object.end(); ++it)
                {
                    auto collection_name = std::string(it->first.get());
                    const auto count = static_cast<std::uint64_t>(it->second.get<double>());
                    if(!elem.task_package_reward_map.emplace(std::move(collection_name), count).second)
                    {
                        LOG_EMPERY_CENTER_ERROR("Duplicate task_package_reward_map : collection_name = ", collection_name);
                        DEBUG_THROW(Exception, sslit("Duplicate task_package_reward_map "));
                    }
                }

                //军团礼包分享奖励
                csv.get(object, "reward2_id");
                elem.share_package_reward_map.reserve(object.size());

                for(auto it = object.begin(); it != object.end(); ++it)
                {
                    auto collection_name = std::string(it->first.get());
                    const auto count = static_cast<std::uint64_t>(it->second.get<double>());
                    if(!elem.share_package_reward_map.emplace(std::move(collection_name), count).second)
                    {
                        LOG_EMPERY_CENTER_ERROR("Duplicate share_package_reward_map : collection_name = ", collection_name);
                        DEBUG_THROW(Exception, sslit("Duplicate share_package_reward_map "));
                    }
                }
                LOG_EMPERY_CENTER_ERROR("Load  corps_box.csv : success!!!");
            }
            g_legionPackageCorpsBox_container = legionPackageCorpsBox_container;
            handles.push(legionPackageCorpsBox_container);
            auto servlet = DataSession::create_servlet(CORPS_BOX_FILE,Data::encode_csv_as_json(csv,"task_id"));
            handles.push(std::move(servlet));
        }
    }

    namespace Data
    {
        boost::shared_ptr<const LegionPackageCorpsBox> LegionPackageCorpsBox::get(TaskId task_id)
        {
           PROFILE_ME;

		   const auto legionPackageCorpsBox_container = g_legionPackageCorpsBox_container.lock();
		   if(!legionPackageCorpsBox_container)
           {
			 LOG_EMPERY_CENTER_WARNING("LegionPackageCorpsBoxContainer has not been loaded.");
			 return { };
		   }

		   const auto it = legionPackageCorpsBox_container->find<0>(task_id);
		   if(it == legionPackageCorpsBox_container->end<0>())
           {
			 LOG_EMPERY_CENTER_TRACE("corps_box not found: task_id = ", task_id);
			 return { };
		   }
		   return boost::shared_ptr<const LegionPackageCorpsBox>(legionPackageCorpsBox_container, &*it);
        }

	    boost::shared_ptr<const LegionPackageCorpsBox> LegionPackageCorpsBox::require(TaskId task_id)
        {
           PROFILE_ME;
		   auto ret = get(task_id);
		   if(!ret)
           {
			 LOG_EMPERY_CENTER_WARNING("corps_box not found: task_id = ", task_id);
			 DEBUG_THROW(Exception, sslit("corps_box not found"));
		   }
		   return ret;
        }

         bool LegionPackageCorpsBox::check_legion_task_package(TaskId task_id,std::uint64_t package_id,std::uint64_t legion_level)
         {
             auto  ret = get(task_id);
             if (!ret)
             {
                 return false;
             }

             auto it =  ret->task_package_reward_map.find(boost::lexical_cast<std::string>(package_id));
             if(it == ret->task_package_reward_map.end())
             {
                return false;
             }

             if(legion_level != it->second)
             {
                return false;
             }

             return true;
         }

         bool LegionPackageCorpsBox::check_legion_share_package(TaskId task_id,std::uint64_t package_id,std::uint64_t legion_level)
         {
             auto  ret = get(task_id);
             if (!ret)
             {
                 return false;
             }

             auto it =  ret->share_package_reward_map.find(boost::lexical_cast<std::string>(package_id));
             if(it == ret->share_package_reward_map.end())
             {
                return false;
             }

             if(legion_level != it->second)
             {
                return false;
             }

             return true;
         }
    }
}