#ifndef EMPERY_CENTER_SINGLETONS_ACCOUNT_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_ACCOUNT_MAP_HPP_

#include <string>
#include <vector>
#include <cstddef>
#include <boost/shared_ptr.hpp>
#include "../id_types.hpp"

namespace EmperyCenter {

class Account;
class PlayerSession;

struct AccountMap {
	static boost::shared_ptr<Account> get(AccountUuid account_uuid);
	static boost::shared_ptr<Account> require(AccountUuid account_uuid);

	static boost::shared_ptr<Account> get_by_login_name(PlatformId platform_id, const std::string &login_name);
	static boost::shared_ptr<Account> require_by_login_name(PlatformId platform_id, const std::string &login_name);

	static void get_all(std::vector<boost::shared_ptr<Account>> &ret);
	static void get_by_nick(std::vector<boost::shared_ptr<Account>> &ret, const std::string &nick);

	static void insert(const boost::shared_ptr<Account> &account, const std::string &remote_ip);
	static void update(const boost::shared_ptr<Account> &account, bool throws_if_not_exists = true);

	static void synchronize_account_with_player(AccountUuid account_uuid, const boost::shared_ptr<PlayerSession> &session,
		bool wants_nick, bool wants_attributes, bool wants_private_attributes, bool wants_items) noexcept;
	static void cached_synchronize_account_with_player(AccountUuid account_uuid, const boost::shared_ptr<PlayerSession> &session) noexcept;

private:
	AccountMap() = delete;
};

}

#endif
