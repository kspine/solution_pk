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
	static boost::shared_ptr<Account> reload(AccountUuid account_uuid);
	static void update(const boost::shared_ptr<Account> &account, bool throws_if_not_exists = true);

	static void get_all_controllers(std::vector<boost::shared_ptr<ControllerSession>> &ret);

private:
	AccountMap() = delete;
};

}

#endif
