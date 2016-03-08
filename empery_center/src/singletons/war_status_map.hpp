#ifndef EMPERY_CENTER_SINGLETONS_WAR_STATUS_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_WAR_STATUS_MAP_HPP_

#include "../id_types.hpp"
#include <vector>
#include <cstdint>

namespace EmperyCenter {

class PlayerSession;

struct WarStatusMap {
	struct StatusInfo {
		AccountUuid less_account_uuid;
		AccountUuid greater_account_uuid;
		std::uint64_t expiry_time;
	};

	static StatusInfo get(AccountUuid first_account_uuid, AccountUuid second_account_uuid);
	static void get_all(std::vector<StatusInfo> &ret, AccountUuid account_uuid);

	static void set(AccountUuid first_account_uuid, AccountUuid second_account_uuid, std::uint64_t expiry_time);

	static void synchronize_with_player_by_account(const boost::shared_ptr<PlayerSession> &session, AccountUuid account_uuid);

private:
	WarStatusMap() = delete;
};

}

#endif
