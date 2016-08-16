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
        static boost::shared_ptr<MySql::Center_Legion_Package_Share> find(LegionUuid legion_uuid,
            AccountUuid account_uuid,
            std::uint64_t task_id,
            std::uint64_t share_package_item_id,
            std::uint64_t share_package_time);

		static void insert(const boost::shared_ptr<MySql::Center_Legion_Package_Share> &shares);

		static void get_by_legion_uuid(std::vector<boost::shared_ptr<MySql::Center_Legion_Package_Share>> &ret, LegionUuid legion_uuid);

		static std::uint64_t get_share_package_time(LegionUuid legion_uuid,AccountUuid account_uuid,std::uint64_t task_id,std::uint64_t share_package_item_id);

	    public:
			enum EIndex
			{
				EIndex_legion_uuid,
				EIndex_account_uuid,
				EIndex_task_id,
				EIndex_share_package_item_id,
				EIndex_share_package_time
			};

        private:
          LegionPackageShareMap() = delete;
    };
}

#endif//