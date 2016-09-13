#ifndef EMPERY_CENTER_SINGLETONS_LEGION_INVITEJOIN_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_LEGION_INVITEJOIN_MAP_HPP_

#include <string>
#include <vector>
#include <cstddef>
#include <boost/shared_ptr.hpp>
#include "../id_types.hpp"

namespace EmperyCenter {


namespace MySql {
	class Center_LegionInviteJoin;
}

class PlayerSession;


struct LegionInviteJoinMap {

	static boost::shared_ptr<MySql::Center_LegionInviteJoin> find(AccountUuid account_uuid,LegionUuid legion_uuid,AccountUuid invited_uuid);

	static void get_by_invited_uuid(std::vector<boost::shared_ptr<MySql::Center_LegionInviteJoin>> &ret, AccountUuid invited_uuid);

	static boost::shared_ptr<MySql::Center_LegionInviteJoin> find_inviteinfo_by_user(AccountUuid account_uuid,LegionUuid legion_uuid);

	static std::uint64_t get_apply_count(AccountUuid account_uuid);

	static void insert(const boost::shared_ptr<MySql::Center_LegionInviteJoin> &account);
	static void deleteInfo(LegionUuid legion_uuid,AccountUuid account_uuid,bool bAll = false);

	static void deleteInfo_by_invited_uuid(AccountUuid account_uuid);
	static void deleteInfo_by_legion_uuid(LegionUuid legion_uuid);
	static void deleteInfo_by_invitedres_uuid(AccountUuid account_uuid,LegionUuid legion_uuid,bool bAll = false);

	static void synchronize_with_player(LegionUuid legion_uuid,const boost::shared_ptr<PlayerSession> &session);

private:
	LegionInviteJoinMap() = delete;
};

}

#endif
