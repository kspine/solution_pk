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
			LegionPackageShareUuid share_uuid;
			LegionUuid legion_uuid;
			AccountUuid account_uuid;
			std::uint64_t task_id;
			std::uint64_t task_package_item_id;
			std::uint64_t share_package_item_id;
			std::uint64_t share_package_time;
			std::uint64_t share_package_expire_time;

			explicit LegionPackageShareElement(boost::shared_ptr<MySql::Center_Legion_Package_Share> shares_)
				: shares(std::move(shares_))
				, share_uuid(shares->get_share_uuid())
				, legion_uuid(shares->get_legion_uuid())
				, account_uuid(shares->get_account_uuid())
				, task_id(shares->get_task_id())
				, task_package_item_id(shares->get_task_package_item_id())
				, share_package_item_id(shares->get_share_package_item_id())
				, share_package_time(shares->get_share_package_time())
				, share_package_expire_time(shares->get_share_package_expire_time()) {}
		};

		MULTI_INDEX_MAP(LegionPackageShareContainer, LegionPackageShareElement,
			UNIQUE_MEMBER_INDEX(share_uuid)
			MULTI_MEMBER_INDEX(legion_uuid)
			MULTI_MEMBER_INDEX(account_uuid)
			MULTI_MEMBER_INDEX(task_id)
			MULTI_MEMBER_INDEX(task_package_item_id)
			MULTI_MEMBER_INDEX(share_package_item_id)
			MULTI_MEMBER_INDEX(share_package_time)
			MULTI_MEMBER_INDEX(share_package_expire_time)
		)

			boost::shared_ptr<LegionPackageShareContainer> g_legion_package_share_map;

		MODULE_RAII_PRIORITY(handles, 5000)
		{
			const auto conn = Poseidon::MySqlDaemon::create_connection();

			struct TempLegionPackageShareElement
			{
				boost::shared_ptr<MySql::Center_Legion_Package_Share> obj;
			};

			std::map<LegionPackageShareUuid, TempLegionPackageShareElement> temp_map;

			LOG_EMPERY_CENTER_INFO("Loading Center_Legion_Package_Share...");

			std::string str_query = "SELECT * FROM `Center_Legion_Package_Share` ";
			std::string str_condition = " WHERE DATE(share_package_time) = CURDATE()";
			str_query += str_condition;

			conn->execute_sql(str_query);

			while (conn->fetch_row())
			{
				auto obj = boost::make_shared<MySql::Center_Legion_Package_Share>();
				obj->fetch(conn);
				obj->enable_auto_saving();

				const auto share_uuid = LegionPackageShareUuid(obj->get_share_uuid());
				temp_map[share_uuid].obj = std::move(obj);
			}

			LOG_EMPERY_CENTER_INFO("Loaded ", temp_map.size(), " LegionPackage(s).");

			const auto shares_map = boost::make_shared<LegionPackageShareContainer>();
			for (auto it = temp_map.begin(); it != temp_map.end(); ++it)
			{
				shares_map->insert(LegionPackageShareElement(std::move(it->second.obj)));
			}
			g_legion_package_share_map = shares_map;
			handles.push(shares_map);
		}
	}

	boost::shared_ptr<MySql::Center_Legion_Package_Share> LegionPackageShareMap::find(LegionPackageShareUuid share_uuid)
	{
		PROFILE_ME;
		const auto &shares_map = g_legion_package_share_map;
		if (!shares_map)
		{
			return{};
		}

		const auto range = shares_map->equal_range<0>(share_uuid);
		for (auto it = range.first; it != range.second; ++it)
		{
			return it->shares;
		}

		return{};
	}

	void LegionPackageShareMap::insert(const boost::shared_ptr<MySql::Center_Legion_Package_Share> &shares)
	{
		PROFILE_ME;

		const auto &shares_map = g_legion_package_share_map;
		if (!shares_map)
		{
			return;
		}

		if (!shares_map->insert(LegionPackageShareElement(shares)).second)
		{
			return;
		}
	}

	void LegionPackageShareMap::get_by_legion_uuid(std::vector<boost::shared_ptr<MySql::Center_Legion_Package_Share>> &ret, LegionUuid legion_uuid)
	{
		PROFILE_ME;
		const auto &shares_map = g_legion_package_share_map;
		if (!shares_map)
		{
			return;
		}

		const auto range = shares_map->equal_range<EIndex_legion_uuid>(legion_uuid);
		ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
		for (auto it = range.first; it != range.second; ++it)
		{
			ret.emplace_back(it->shares);
		}

		return;
	}

	bool LegionPackageShareMap::check(AccountUuid account_uuid, TaskId task_id, std::uint64_t task_package_item_id)
	{
		PROFILE_ME;
		const auto &shares_map = g_legion_package_share_map;
		if (!shares_map)
		{
			return false;
		}

		const auto range = shares_map->equal_range<EIndex_account_uuid>(account_uuid);
		for (auto it = range.first; it != range.second; ++it)
		{
			if (TaskId(it->shares->unlocked_get_task_id()) == task_id)
			{
				if ((it->shares->unlocked_get_task_package_item_id()) == task_package_item_id)
				{
					return true;
				}
			}
		}

		return false;
	}

	void LegionPackageShareMap::get_account_uuid(LegionPackageShareUuid share_uuid, AccountUuid& account_uuid)
	{
		PROFILE_ME;

		const auto  pShare = find(share_uuid);
		if (NULL != pShare)
		{
			account_uuid = AccountUuid(pShare->unlocked_get_account_uuid());
		}

		return;
	}

	std::uint64_t LegionPackageShareMap::get_task_id(LegionPackageShareUuid share_uuid)
	{
		PROFILE_ME;

		const auto  pShare = find(share_uuid);
		if (NULL != pShare)
		{
			return pShare->unlocked_get_task_id();
		}

		return 0;
	}
	std::uint64_t LegionPackageShareMap::get_task_package_item_id(LegionPackageShareUuid share_uuid)
	{
		PROFILE_ME;

		const auto  pShare = find(share_uuid);
		if (NULL != pShare)
		{
			return pShare->unlocked_get_task_package_item_id();
		}

		return 0;
	}
	std::uint64_t LegionPackageShareMap::get_share_package_item_id(LegionPackageShareUuid share_uuid)
	{
		PROFILE_ME;

		const auto  pShare = find(share_uuid);
		if (NULL != pShare)
		{
			return pShare->unlocked_get_share_package_item_id();
		}

		return 0;
	}

	std::uint64_t LegionPackageShareMap::get_share_package_time(LegionPackageShareUuid share_uuid)
	{
		PROFILE_ME;

		const auto  pShare = find(share_uuid);
		if (NULL != pShare)
		{
			return pShare->unlocked_get_share_package_time();
		}

		return 0;
	}

	std::uint64_t LegionPackageShareMap::get_share_package_expire_time(LegionPackageShareUuid share_uuid)
	{
		PROFILE_ME;

		const auto  pShare = find(share_uuid);
		if (NULL != pShare)
		{
			return pShare->unlocked_get_share_package_expire_time();
		}

		return 0;
	}
}