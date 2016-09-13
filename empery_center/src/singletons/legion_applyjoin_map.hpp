#ifndef EMPERY_CENTER_SINGLETONS_LEGION_APPLYJOIN_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_LEGION_APPLYJOIN_MAP_HPP_

#include <string>
#include <vector>
#include <cstddef>
#include <boost/shared_ptr.hpp>
#include "../id_types.hpp"

namespace EmperyCenter {


namespace MySql {
	class Center_LegionApplyJoin;
}

class PlayerSession;


struct LegionApplyJoinMap {

	static boost::shared_ptr<MySql::Center_LegionApplyJoin> find(AccountUuid account_uuid,LegionUuid legion_uuid);

	static std::uint64_t get_apply_count(AccountUuid account_uuid);

	static void insert(const boost::shared_ptr<MySql::Center_LegionApplyJoin> &account);
	static void deleteInfo(LegionUuid legion_uuid,AccountUuid account_uuid,bool bAll = false);
	static void deleteInfo_by_legion_uuid(LegionUuid legion_uuid);

	static void synchronize_with_player(LegionUuid legion_uuid,const boost::shared_ptr<PlayerSession> &session);

private:
	LegionApplyJoinMap() = delete;
};

}

#endif
