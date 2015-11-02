#ifndef EMPERY_CENTER_MSG_CERR_ACCOUNT_HPP_
#define EMPERY_CENTER_MSG_CERR_ACCOUNT_HPP_

#include <poseidon/cbpp/status_codes.hpp>

namespace EmperyCenter {

namespace Msg {
	using namespace Poseidon::Cbpp::StatusCodes;

	enum {
		CERR_NO_SUCH_ACCOUNT                = 71201,
		CERR_TOKEN_EXPIRED                  = 71202,
		CERR_ACCOUNT_BANNED                 = 71203,
		CERR_NOT_LOGGED_IN                  = 71204,
		CERR_INVALID_TOKEN                  = 71205,
		CERR_ATTR_NOT_SETTABLE              = 71206,
		CERR_ATTR_TOO_LONG                  = 71207,
		CERR_NO_SUCH_ACCOUNT_NICK           = 71208,
		CERR_NO_SUCH_LOGIN_NAME             = 71209,
		CERR_MULTIPLE_LOGIN                 = 71210,
		CERR_NICK_TOO_LONG                  = 71211,
	};
}

}

#endif
