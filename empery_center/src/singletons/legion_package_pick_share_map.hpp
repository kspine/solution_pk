#ifndef EMPERY_CENTER_SINGLETONS_LEGION_PACKAGE_PICK_SHARE_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_LEGION_PACKAGE_PICK_SHARE_MAP_HPP_

#include <string>
#include <vector>
#include <cstddef>
#include <boost/shared_ptr.hpp>
#include "../id_types.hpp"

namespace EmperyCenter
{
	namespace MySql
	{
		class Center_Legion_Package_Pick_Share;
	}

	class PlayerSession;

	enum EPickShareStatus { EPickShareStatus_UnReceived, EPickShareStatus_Received };

	struct LegionPackagePickShareMap
	{
		static void insert(const boost::shared_ptr<MySql::Center_Legion_Package_Pick_Share> &picks_share_);

		static void get_by_share_uuid(std::vector<boost::shared_ptr<MySql::Center_Legion_Package_Pick_Share>> &ret, AccountUuid account_uuid);

		static bool check_share_package_status(LegionPackageShareUuid share_uuid, AccountUuid account_uuid);

	public:
		enum EIndex
		{
			EIndex_share_uuid,
			EIndex_account_uuid,
			EIndex_share_package_status,
			EIndex_share_package_pick_time
		};
	private:
		LegionPackagePickShareMap() = delete;
	};
}

#endif//