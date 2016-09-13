#include "../precompiled.hpp"
#include "legion_package_pick_share_map.hpp"

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
		struct LegionPackagePickShareElement
		{
			boost::shared_ptr<MySql::Center_Legion_Package_Pick_Share> picks_share;
			LegionPackageShareUuid share_uuid;
			AccountUuid account_uuid;
			std::uint64_t share_package_status;
			std::uint64_t share_package_pick_time;

			explicit LegionPackagePickShareElement(boost::shared_ptr<MySql::Center_Legion_Package_Pick_Share> picks_share_)
				: picks_share(std::move(picks_share_))
				, share_uuid(picks_share->get_share_uuid())
				, account_uuid(picks_share->get_account_uuid())
				, share_package_status(picks_share->get_share_package_status())
				, share_package_pick_time(picks_share->get_share_package_pick_time())
			{
			}
		};

		MULTI_INDEX_MAP(LegionPackagePickShareContainer, LegionPackagePickShareElement,
			MULTI_MEMBER_INDEX(share_uuid)
			MULTI_MEMBER_INDEX(account_uuid)
			MULTI_MEMBER_INDEX(share_package_status)
			MULTI_MEMBER_INDEX(share_package_pick_time)
		)

			boost::shared_ptr<LegionPackagePickShareContainer> g_legion_package_pick_share_map;

		MODULE_RAII_PRIORITY(handles, 5000)
		{
			const auto conn = Poseidon::MySqlDaemon::create_connection();

			struct TempLegionPackagePickShareElement
			{
				boost::shared_ptr<MySql::Center_Legion_Package_Pick_Share> obj;
			};

			std::map<LegionPackageShareUuid, TempLegionPackagePickShareElement> temp_map;

			LOG_EMPERY_CENTER_INFO("Loading Center_Legion_Package_Pick_Share...");

			conn->execute_sql("SELECT * FROM `Center_Legion_Package_Pick_Share`");

			while (conn->fetch_row())
			{
				auto obj = boost::make_shared<MySql::Center_Legion_Package_Pick_Share>();
				obj->fetch(conn);
				obj->enable_auto_saving();

				const auto share_uuid = LegionPackageShareUuid(obj->get_share_uuid());
				temp_map[share_uuid].obj = std::move(obj);
			}

			LOG_EMPERY_CENTER_INFO("Loaded ", temp_map.size(), " LegionPackage(s).");

			const auto picks_share_map = boost::make_shared<LegionPackagePickShareContainer>();
			for (auto it = temp_map.begin(); it != temp_map.end(); ++it)
			{
				picks_share_map->insert(LegionPackagePickShareElement(std::move(it->second.obj)));
			}
			g_legion_package_pick_share_map = picks_share_map;
			handles.push(picks_share_map);
		}
	}

	void LegionPackagePickShareMap::insert(const boost::shared_ptr<MySql::Center_Legion_Package_Pick_Share> &picks_share)
	{
		PROFILE_ME;

		const auto &picks_share_map = g_legion_package_pick_share_map;
		if (!picks_share_map)
		{
			return;
		}

		if (!picks_share_map->insert(LegionPackagePickShareElement(picks_share)).second)
		{
			return;
		}
	}

	bool LegionPackagePickShareMap::check_share_package_status(LegionPackageShareUuid share_uuid, AccountUuid account_uuid)
	{
		PROFILE_ME;
		const auto &picks_share_map = g_legion_package_pick_share_map;
		if (!picks_share_map)
		{
			return false;
		}

		const auto range = picks_share_map->equal_range<0>(share_uuid);
		for (auto it = range.first; it != range.second; ++it)
		{
			if (AccountUuid(it->picks_share->unlocked_get_account_uuid()) == account_uuid)
			{
				if (it->picks_share->unlocked_get_share_package_status() == EPickShareStatus_Received)
				{
					return true;
				}
			}
		}

		return false;
	}

	void LegionPackagePickShareMap::get_by_share_uuid(std::vector<boost::shared_ptr<MySql::Center_Legion_Package_Pick_Share>> &ret, AccountUuid account_uuid)
	{
		PROFILE_ME;
		const auto &picks_share_map = g_legion_package_pick_share_map;
		if (!picks_share_map)
		{
			return;
		}

		const auto range = picks_share_map->equal_range<1>(account_uuid);
		ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
		for (auto it = range.first; it != range.second; ++it)
		{
			ret.emplace_back(it->picks_share);
		}
		return;
	}
}