#ifndef EMPERY_CONTROLLER_SINGLETONS_ACCOUNT_MAP_HPP_
#define EMPERY_CONTROLLER_SINGLETONS_ACCOUNT_MAP_HPP_

#include <boost/shared_ptr.hpp>
#include <vector>
#include "../id_types.hpp"

namespace EmperyController {

class Account;
class ControllerSession;

struct AccountMap {
	static boost::shared_ptr<Account> get(AccountUuid account_uuid);
	static boost::shared_ptr<Account> require(AccountUuid account_uuid);
	static boost::shared_ptr<Account> get_or_reload(AccountUuid account_uuid);
	static boost::shared_ptr<Account> forced_reload(AccountUuid account_uuid);
	static boost::shared_ptr<Account> get_or_reload_by_login_name(PlatformId platform_id, const std::string &login_name);

	static void update(const boost::shared_ptr<Account> &account, bool throws_if_not_exists = true);

	static void get_all_controllers(std::vector<boost::shared_ptr<ControllerSession>> &ret);

private:
	AccountMap() = delete;
};

}

#endif
