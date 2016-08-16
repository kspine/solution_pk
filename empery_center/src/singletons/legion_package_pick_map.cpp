
#include "../precompiled.hpp"
#include "legion_package_pick_map.hpp"

#include "../mmain.hpp"

#include <poseidon/multi_index_map.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/singletons/timer_daemon.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>

#include <tuple>

#include "../player_session.hpp"
#include "../account.hpp"
#include "../string_utilities.hpp"

#include "../mysql/legion.hpp"
#include "account_map.hpp"
#include "../account_attribute_ids.hpp"

namespace EmperyCenter
{
    namespace
    {
         struct LegionPackagePickElement
         {
            boost::shared_ptr<MySql::Center_Legion_Package_Pick> picks;

            LegionUuid legion_uuid;
            AccountUuid account_uuid;
            std::uint64_t task_id;
            std::uint64_t task_package_item_id;
            std::uint64_t share_package_item_id;
            std::uint64_t task_package_status;
            std::uint64_t share_package_status;

            explicit LegionPackagePickElement(boost::shared_ptr<MySql::Center_Legion_Package_Pick> picks_)
                            : picks(std::move(picks_))
                            ,legion_uuid(picks->get_legion_uuid())
                            ,account_uuid(picks->get_account_uuid())
                            ,task_id(picks->get_task_id())
                            ,task_package_item_id(picks->get_task_package_item_id())
                            ,share_package_item_id(picks->get_share_package_item_id())
                            ,task_package_status(picks->get_task_package_status())
                            ,share_package_status(picks->get_share_package_status()){}
        };

        MULTI_INDEX_MAP(LegionPackagePickContainer, LegionPackagePickElement,
            MULTI_MEMBER_INDEX(legion_uuid)
            MULTI_MEMBER_INDEX(account_uuid)
            MULTI_MEMBER_INDEX(task_id)
            MULTI_MEMBER_INDEX(task_package_item_id)
            MULTI_MEMBER_INDEX(share_package_item_id)
            MULTI_MEMBER_INDEX(task_package_status)
            MULTI_MEMBER_INDEX(share_package_status)
        )

        boost::shared_ptr<LegionPackagePickContainer> g_legion_package_pick_map;

        MODULE_RAII_PRIORITY(handles, 5000)
        {
            const auto conn = Poseidon::MySqlDaemon::create_connection();

            struct TempLegionPackagePickElement
            {
                boost::shared_ptr<MySql::Center_Legion_Package_Pick> obj;
            };

            std::map<LegionUuid, TempLegionPackagePickElement> temp_account_map;

            LOG_EMPERY_CENTER_INFO("Loading Center_Legion_Package_Pick...");

            conn->execute_sql("SELECT * FROM `Center_Legion_Package_Pick`");

            while(conn->fetch_row())
            {
                auto obj = boost::make_shared<MySql::Center_Legion_Package_Pick>();
                obj->fetch(conn);
                obj->enable_auto_saving();
                const auto legion_uuid = LegionUuid(obj->get_legion_uuid());
                temp_account_map[legion_uuid].obj = std::move(obj);
            }

            LOG_EMPERY_CENTER_INFO("Loaded ", temp_account_map.size(), " LegionPackage(s).");

            const auto legion_package_pick_map = boost::make_shared<LegionPackagePickContainer>();
            for(auto it = temp_account_map.begin(); it != temp_account_map.end(); ++it)
			{
                legion_package_pick_map->insert(LegionPackagePickElement(std::move(it->second.obj)));
            }
            g_legion_package_pick_map = legion_package_pick_map;
            handles.push(legion_package_pick_map);
        }
    }

    boost::shared_ptr<MySql::Center_Legion_Package_Pick> LegionPackagePickMap::find(LegionUuid legion_uuid,
            AccountUuid account_uuid,
            std::uint64_t task_id,
            std::uint64_t task_package_item_id,
            std::uint64_t share_package_item_id,
            std::uint64_t task_package_status,
            std::uint64_t share_package_status)
            {
                PROFILE_ME;
                const auto &account_map = g_legion_package_pick_map;
                if(!account_map)
                {
                    LOG_EMPERY_CENTER_WARNING("g_legion_package_pick_map not loaded.");
                    return { };
                }

                LOG_EMPERY_CENTER_DEBUG("g_legion_package_pick_map find:"
                "  legion_uuid:",legion_uuid,
                "  account_uuid = ", account_uuid,
                "  task_id:",task_id,
                "  task_package_item_id:",task_package_item_id,
                "  share_package_item_id:",share_package_item_id,
                "  task_package_status:",task_package_status,
                "  share_package_status:",share_package_status);

                const auto range = account_map->equal_range<EIndex_legion_uuid>(legion_uuid);
                for(auto it = range.first; it != range.second; ++it)
                {
                    if(LegionUuid(it->picks->unlocked_get_legion_uuid()) == legion_uuid &&
                       AccountUuid(it->picks->unlocked_get_account_uuid()) == account_uuid &&
                       it->picks->unlocked_get_task_id() == task_id &&
                       it->picks->unlocked_get_task_package_item_id() == task_package_item_id &&
                       it->picks->unlocked_get_share_package_item_id() == share_package_item_id &&
                       it->picks->unlocked_get_task_package_status() == task_package_status &&
                       it->picks->unlocked_get_share_package_status() == share_package_status)
                    {
                        LOG_EMPERY_CENTER_WARNING("g_legion_package_pick_map  Find.");
                        return it->picks;
                    }
                }
                return { };
            }


    void LegionPackagePickMap::insert(const boost::shared_ptr<MySql::Center_Legion_Package_Pick> &picks)
    {
         PROFILE_ME;

		 const auto &legion_package_pick_map = g_legion_package_pick_map;
		 if (!legion_package_pick_map)
		 {
			 LOG_EMPERY_CENTER_WARNING("LegionPackagePickMap  not loaded.");
			 DEBUG_THROW(Exception, sslit("LegionPackagePickMap not loaded"));
		 }

		 const auto legion_uuid = picks->get_legion_uuid();
		 const auto account_uuid = picks->get_account_uuid();
		 LOG_EMPERY_CENTER_DEBUG("Inserting Center_Legion_Package_Pick: legion_uuid = ", legion_uuid);
		 LOG_EMPERY_CENTER_DEBUG("Inserting Center_Legion_Package_Pick: account_uuid = ", account_uuid);

		 if (!legion_package_pick_map->insert(LegionPackagePickElement(picks)).second)
		 {
			 LOG_EMPERY_CENTER_WARNING("Center_Legion_Package_Pick already exists: legion_uuid = ", legion_uuid);
			 DEBUG_THROW(Exception, sslit("Center_Legion_Package_Pick already exists"));
		 }
    }

	bool LegionPackagePickMap::check_task_package_status(LegionUuid legion_uuid, AccountUuid account_uuid, std::uint64_t task_id, std::uint64_t task_package_item_id)
	{
		PROFILE_ME;

		const auto &legion_package_pick_map = g_legion_package_pick_map;
		if (!legion_package_pick_map)
		{
			LOG_EMPERY_CENTER_WARNING("g_legion_package_pick_map  not loaded.");
			return false;
		}

		const auto range = legion_package_pick_map->equal_range<EIndex_legion_uuid>(legion_uuid);
		for (auto it = range.first; it != range.second; ++it)
		{
			LOG_EMPERY_CENTER_DEBUG("check_task_package_status ********************** ",
				"  legion_uuid:", LegionUuid(it->picks->unlocked_get_legion_uuid()),
				"  AccountUuid:", AccountUuid(it->picks->unlocked_get_account_uuid()));


			if (LegionUuid(it->picks->unlocked_get_legion_uuid()) == legion_uuid &&
				AccountUuid(it->picks->unlocked_get_account_uuid()) == account_uuid &&
				(it->picks->unlocked_get_task_id())== task_id &&
				(it->picks->unlocked_get_task_package_item_id())== task_package_item_id)
			{
				if (it->picks->unlocked_get_task_package_status() == EPickStatus_Received)
				{
					LOG_EMPERY_CENTER_DEBUG("check_task_package_status ********************** ",
						"  legion_uuid:", LegionUuid(it->picks->unlocked_get_legion_uuid()),
						"  AccountUuid:", AccountUuid(it->picks->unlocked_get_account_uuid()),
						" task_package_status:", it->picks->unlocked_get_task_package_status());

					return true;
				}
			}
		}
		return false;
	}


	bool LegionPackagePickMap::check_share_package_status(LegionUuid legion_uuid, AccountUuid account_uuid, std::uint64_t task_id, std::uint64_t share_package_item_id)
	{
		PROFILE_ME;

		const auto &legion_package_pick_map = g_legion_package_pick_map;
		if (!legion_package_pick_map)
		{
			LOG_EMPERY_CENTER_WARNING("g_legion_package_pick_map  not loaded.");
			return false;
		}

		const auto range = legion_package_pick_map->equal_range<EIndex_legion_uuid>(legion_uuid);

		for (auto it = range.first; it != range.second; ++it)
		{
			LOG_EMPERY_CENTER_DEBUG("check_share_package_status ********************** ",
				"  legion_uuid:", LegionUuid(it->picks->unlocked_get_legion_uuid()),
				"  AccountUuid:", AccountUuid(it->picks->unlocked_get_account_uuid()));


			if (LegionUuid(it->picks->unlocked_get_legion_uuid()) == legion_uuid &&
				AccountUuid(it->picks->unlocked_get_account_uuid()) == account_uuid &&
				(it->picks->unlocked_get_task_id()) == task_id &&
				(it->picks->unlocked_get_share_package_item_id()) == share_package_item_id)
			{
				if (it->picks->unlocked_get_share_package_status() == EPickStatus_Received)
				{
					LOG_EMPERY_CENTER_DEBUG("check_share_package_status ********************** ",
						"  legion_uuid:", LegionUuid(it->picks->unlocked_get_legion_uuid()),
						"  AccountUuid:", AccountUuid(it->picks->unlocked_get_account_uuid()),
						" task_package_status:", it->picks->unlocked_get_share_package_status());

					return true;
				}
			}
		}
		return false;
	}

    std::uint64_t LegionPackagePickMap::get_share_package_status(LegionUuid legion_uuid, AccountUuid account_uuid, std::uint64_t task_id, std::uint64_t share_package_item_id)
	{
		PROFILE_ME;

		const auto &legion_package_pick_map = g_legion_package_pick_map;
		if (legion_package_pick_map)
		{
			const auto range = legion_package_pick_map->equal_range<EIndex_legion_uuid>(legion_uuid);

		    for (auto it = range.first; it != range.second; ++it)
	    	{
		    	LOG_EMPERY_CENTER_DEBUG("get_share_package_status ********************** ",
				"  legion_uuid:", LegionUuid(it->picks->unlocked_get_legion_uuid()),
				"  AccountUuid:", AccountUuid(it->picks->unlocked_get_account_uuid()));


			   if (LegionUuid(it->picks->unlocked_get_legion_uuid()) == legion_uuid &&
				   AccountUuid(it->picks->unlocked_get_account_uuid()) == account_uuid &&
			    	(it->picks->unlocked_get_task_id()) == task_id &&
			    	(it->picks->unlocked_get_share_package_item_id()) == share_package_item_id)
			    {
				   if (it->picks->unlocked_get_share_package_status() == EPickStatus_Received)
				   {
				      return (uint64_t)EPickStatus_Received;
				   }

				   return (uint64_t)EPickStatus_UnReceived;
			    }
	     	}

		    return (uint64_t)EPickStatus_UnReceived;
     	}

        return (uint64_t)EPickStatus_UnReceived;
     }
}