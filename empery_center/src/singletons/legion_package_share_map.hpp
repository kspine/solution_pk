#ifndef EMPERY_CENTER_SINGLETONS_LEGION_PACKAGE_SHARE_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_LEGION_PACKAGE_SHARE_MAP_HPP_

#include <string>
#include <vector>
#include <cstddef>
#include <boost/shared_ptr.hpp>
#include "../id_types.hpp"

namespace EmperyCenter
{
	namespace MySql
	{
		class Center_Legion_Package_Share;
	}

	class PlayerSession;

	struct LegionPackageShareMap
	{
		static boost::shared_ptr<MySql::Center_Legion_Package_Share> find(LegionPackageShareUuid share_uuid);

		static void insert(const boost::shared_ptr<MySql::Center_Legion_Package_Share> &shares);

		static void get_by_legion_uuid(std::vector<boost::shared_ptr<MySql::Center_Legion_Package_Share>> &ret, LegionUuid legion_uuid);

		static bool check(AccountUuid account_uuid, TaskId task_id, std::uint64_t task_package_item_id);

		static void get_account_uuid(LegionPackageShareUuid share_uuid, AccountUuid& account_uuid);
		static std::uint64_t get_task_id(LegionPackageShareUuid share_uuid);
		static std::uint64_t get_task_package_item_id(LegionPackageShareUuid share_uuid);
		static std::uint64_t get_share_package_item_id(LegionPackageShareUuid share_uuid);
		static std::uint64_t get_share_package_time(LegionPackageShareUuid share_uuid);
		static std::uint64_t get_share_package_expire_time(LegionPackageShareUuid share_uuid);

	public:
		enum EIndex
		{
			EIndex_share_uuid,
			EIndex_legion_uuid,
			EIndex_account_uuid,
			EIndex_task_id,
			EIndex_task_package_item_id,
			EIndex_share_package_item_id,
			EIndex_share_package_time,
			EIndex_share_package_expire_time
		};

	private:
		LegionPackageShareMap() = delete;
	};
}

#endif//