#ifndef EMPERY_CENTER_SINGLETONS_ACCOUNT_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_ACCOUNT_MAP_HPP_

#include <string>
#include <vector>
#include <cstddef>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include "../id_types.hpp"

namespace EmperyCenter {

class PlayerSession;

struct AccountMap {
	enum {
		FL_VALID                    = 0x0001,
		FL_ROBOT                    = 0x0002,

		ATTR_CUSTOM_PUBLIC_END      = 100,
		ATTR_CUSTOM_END             = 200,
		ATTR_PUBLIC_END             = 300,
		ATTR_END                    = 400,

		MAX_ATTR_LEN                = 4096,

		ATTR_GENDER                 =   1,
		ATTR_AVATAR                 =   2,

		ATTR_TIME_LAST_LOGGED_IN    = 300,
		ATTR_TIME_LAST_LOGGED_OUT   = 301,
		ATTR_TIME_CREATED           = 302,
		ATTR_VIP_LEVEL              = 303,
	};

	struct AccountInfo {
		AccountUuid accountUuid;
		std::string loginName;
		std::string nick;
		boost::uint64_t flags;
		boost::uint64_t createdTime;
	};

private:
	AccountMap();
};

}

#endif
