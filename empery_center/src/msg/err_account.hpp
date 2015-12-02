#ifndef EMPERY_CENTER_MSG_ERR_ACCOUNT_HPP_
#define EMPERY_CENTER_MSG_ERR_ACCOUNT_HPP_

#include <poseidon/cbpp/status_codes.hpp>

namespace EmperyCenter {

namespace Msg {
	using namespace Poseidon::Cbpp::StatusCodes;

	enum {
		ERR_NO_SUCH_ACCOUNT                 = 71201,
		ERR_TOKEN_EXPIRED                   = 71202,
		ERR_ACCOUNT_BANNED                  = 71203,
		ERR_NOT_LOGGED_IN                   = 71204,
		ERR_INVALID_TOKEN                   = 71205,
		ERR_ATTR_NOT_SETTABLE               = 71206,
		ERR_ATTR_TOO_LONG                   = 71207,
		ERR_NO_SUCH_ACCOUNT_NICK            = 71208,
		ERR_NO_SUCH_LOGIN_NAME              = 71209,
		ERR_MULTIPLE_LOGIN                  = 71210,
		ERR_NISK_TOO_LONG                   = 71211,
		ERR_NO_SUCH_SIGN_IN_REWARD          = 71212,
		ERR_ALREADY_SIGNED_IN               = 71213,
	};
}

}

#endif
