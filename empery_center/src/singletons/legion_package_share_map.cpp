#include "../precompiled.hpp"
#include "legion_package_share_map.hpp"

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
         struct LegionPackageShareElement
         {
            boost::shared_ptr<MySql::Center_Legion_Package_Share> shares;

            LegionUuid legion_uuid;
            AccountUuid account_uuid;
            std::uint64_t task_id;
            std::uint64_t share_package_item_id;
            std::uint64_t share_package_time;

            explicit LegionPackageShareElement(boost::shared_ptr<MySql::Center_Legion_Package_Share> shares_)
                            : shares(std::move(shares_))
                            , legion_uuid(shares->get_legion_uuid())
                            , account_uuid(shares->get_account_uuid())
                            , task_id(shares->get_task_id())
                            , share_package_item_id(shares->get_share_package_item_id())
                            , share_package_time(shares->get_share_package_time()) {}
        };

        MULTI_INDEX_MAP(LegionPackageShareContainer, LegionPackageShareElement,
            MULTI_MEMBER_INDEX(legion_uuid)
            MULTI_MEMBER_INDEX(account_uuid)
            MULTI_MEMBER_INDEX(task_id)
            MULTI_MEMBER_INDEX(share_package_item_id)
            MULTI_MEMBER_INDEX(share_package_time)
        )

        boost::shared_ptr<LegionPackageShareContainer> g_legion_package_share_map;

        MODULE_RAII_PRIORITY(handles, 5000)
        {
            const auto conn = Poseidon::MySqlDaemon::create_connection();

            struct TempLegionPackageShareElement
            {
                boost::shared_ptr<MySql::Center_Legion_Package_Share> obj;
            };

            std::map<LegionUuid, TempLegionPackageShareElement> temp_account_map;

            LOG_EMPERY_CENTER_INFO("Loading Center_Legion_Package_Share...");

            conn->execute_sql("SELECT * FROM `Center_Legion_Package_Share`");

            while(conn->fetch_row())
            {
                LOG_EMPERY_CENTER_ERROR("SELECT * FROM `Center_Legion_Package_Share`");

                auto obj = boost::make_shared<MySql::Center_Legion_Package_Share>();
                obj->fetch(conn);
                obj->enable_auto_saving();
                const auto legion_uuid = LegionUuid(obj->get_legion_uuid());
                temp_account_map[legion_uuid].obj = std::move(obj);
            }

            LOG_EMPERY_CENTER_INFO("Loaded ", temp_account_map.size(), " LegionPackage(s).");
            const auto legion_package_share_map = boost::make_shared<LegionPackageShareContainer>();
            for(auto it = temp_account_map.begin(); it != temp_account_map.end(); ++it)
			{
                legion_package_share_map->insert(LegionPackageShareElement(std::move(it->second.obj)));
            }
            g_legion_package_share_map = legion_package_share_map;
            handles.push(legion_package_share_map);
        }
    }


    boost::shared_ptr<MySql::Center_Legion_Package_Share> LegionPackageShareMap::find(LegionUuid legion_uuid,
            AccountUuid account_uuid,
            std::uint64_t task_id,
            std::uint64_t share_package_item_id,
            std::uint64_t share_package_time)
            {
                PROFILE_ME;
                const auto &account_map = g_legion_package_share_map;
                if(!account_map)
                {
                    LOG_EMPERY_CENTER_WARNING("g_legion_package_share_map not loaded.");
                    return { };
                }

                LOG_EMPERY_CENTER_DEBUG("LegionPackageShareMap find:"
                "  legion_uuid:",legion_uuid,
                "  account_uuid = ", account_uuid,
                "  task_id:",task_id,
                "  share_package_item_id:",share_package_item_id,
                "  share_package_time:",share_package_time);

                const auto range = account_map->equal_range<EIndex_legion_uuid>(legion_uuid);
                for(auto it = range.first; it != range.second; ++it)
                {
                    if(LegionUuid(it->shares->unlocked_get_legion_uuid()) == legion_uuid &&
                       AccountUuid(it->shares->unlocked_get_account_uuid()) == account_uuid &&
                       it->shares->unlocked_get_task_id() == task_id &&
                       it->shares->unlocked_get_share_package_item_id() == share_package_item_id &&
                       it->shares->unlocked_get_share_package_time() == share_package_time)
                    {
                        LOG_EMPERY_CENTER_WARNING("g_legion_package_share_map  Find.");
                        return it->shares;
                    }
                }
                return { };
            }

        void LegionPackageShareMap::insert(const boost::shared_ptr<MySql::Center_Legion_Package_Share> &shares)
        {
            PROFILE_ME;

            const auto &legion_package_share_map = g_legion_package_share_map;
            if(!legion_package_share_map)
            {
                LOG_EMPERY_CENTER_WARNING("LegionPackageShareMap  not loaded.");
                 DEBUG_THROW(Exception, sslit("LegionPackageShareMap not loaded"));
            }

            const auto legion_uuid = shares->get_legion_uuid();
            const auto account_uuid = shares->get_account_uuid();
            LOG_EMPERY_CENTER_DEBUG("Inserting Center_Legion_Package_Share: legion_uuid = ", legion_uuid);
            LOG_EMPERY_CENTER_DEBUG("Inserting Center_Legion_Package_Share: account_uuid = ", account_uuid);

            if(!legion_package_share_map->insert(LegionPackageShareElement(shares)).second)
            {
                 LOG_EMPERY_CENTER_WARNING("Center_Legion_Package_Share already exists: legion_uuid = ", legion_uuid);
                 DEBUG_THROW(Exception, sslit("Center_Legion_Package_Share already exists"));
            }
        }

		void LegionPackageShareMap::get_by_legion_uuid(std::vector<boost::shared_ptr<MySql::Center_Legion_Package_Share>> &ret, LegionUuid legion_uuid)
		{
			PROFILE_ME;
			const auto &legion_package_share_map = g_legion_package_share_map;
			if (!legion_package_share_map)
			{
				LOG_EMPERY_CENTER_WARNING("g_legion_package_share_map  not loaded.");

				return;
			}

			const auto range = legion_package_share_map->equal_range<EIndex_legion_uuid>(legion_uuid);
			ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
			for (auto it = range.first; it != range.second; ++it)
			{
				ret.emplace_back(it->shares);
			}
		}


		std::uint64_t LegionPackageShareMap::get_share_package_time(LegionUuid legion_uuid, AccountUuid account_uuid, std::uint64_t task_id, std::uint64_t share_package_item_id) 
		{
			const auto &legion_package_share_map = g_legion_package_share_map;
			if (!legion_package_share_map)
			{
				LOG_EMPERY_CENTER_WARNING("g_legion_package_share_map  not loaded.");

				return 0;
			}

			const auto range = legion_package_share_map->equal_range<EIndex_legion_uuid>(legion_uuid);

			for (auto it = range.first; it != range.second; ++it)
			{
				LOG_EMPERY_CENTER_DEBUG("get_share_package_time ********************** ",
					"  legion_uuid:", LegionUuid(it->shares->unlocked_get_legion_uuid()),
					"  AccountUuid:", AccountUuid(it->shares->unlocked_get_account_uuid()));


				if (LegionUuid(it->shares->unlocked_get_legion_uuid()) == legion_uuid &&
					AccountUuid(it->shares->unlocked_get_account_uuid()) == account_uuid &&
					(it->shares->unlocked_get_task_id()) == task_id &&
					(it->shares->unlocked_get_share_package_item_id()) == share_package_item_id)
				{
					LOG_EMPERY_CENTER_DEBUG("get_share_package_time ********************** ",
						"  legion_uuid:", LegionUuid(it->shares->unlocked_get_legion_uuid()),
						"  AccountUuid:", AccountUuid(it->shares->unlocked_get_account_uuid()),
						"  share_package_time:", it->shares->unlocked_get_share_package_time());

					return it->shares->unlocked_get_share_package_time();
				}
			}
			return 0;
		}

}