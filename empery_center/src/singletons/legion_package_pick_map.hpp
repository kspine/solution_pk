
#ifndef EMPERY_CENTER_SINGLETONS_LEGION_PACKAGE_PICK_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_LEGION_PACKAGE_PICK_MAP_HPP_

#include <string>
#include <vector>
#include <cstddef>
#include <boost/shared_ptr.hpp>
#include "../id_types.hpp"

namespace EmperyCenter
{
    namespace MySql
    {
        class Center_Legion_Package_Pick;
    }

    class PlayerSession;

	enum EPickStatus{ EPickStatus_UnReceived, EPickStatus_Received };

    struct LegionPackagePickMap
    {
        static boost::shared_ptr<MySql::Center_Legion_Package_Pick> find(LegionUuid legion_uuid,
            AccountUuid account_uuid,
            std::uint64_t task_id,
            std::uint64_t task_package_item_id,
            std::uint64_t share_package_item_id,
            std::uint64_t task_package_status,
            std::uint64_t share_package_status);

        static void insert(const boost::shared_ptr<MySql::Center_Legion_Package_Pick> &picks);

		static bool check_task_package_status(LegionUuid legion_uuid,AccountUuid account_uuid,std::uint64_t task_id,std::uint64_t task_package_item_id);

		static bool check_share_package_status(LegionUuid legion_uuid,AccountUuid account_uuid,std::uint64_t task_id,std::uint64_t share_package_item_id);

	    static std::uint64_t  get_share_package_status(LegionUuid legion_uuid,AccountUuid account_uuid,std::uint64_t task_id,std::uint64_t share_package_item_id);

	    public:

			enum EIndex
			{
				EIndex_legion_uuid,
				EIndex_account_uuid,
				EIndex_task_id,
				EIndex_task_package_item_id,
				EIndex_share_package_item_id,
				EIndex_task_package_status,
				EIndex_share_package_status
			};

        private:
          LegionPackagePickMap() = delete;
    };
}

#endif//