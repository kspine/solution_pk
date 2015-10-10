#ifndef EMPERY_CENTER_MSG_ERR_ACCOUNT_HPP_
#define EMPERY_CENTER_MSG_ERR_ACCOUNT_HPP_

#include <poseidon/cbpp/status_codes.hpp>

namespace EmperyCenter {

namespace Msg {
	using namespace Poseidon::Cbpp::StatusCodes;

	enum {
		ERR_NO_SUCH_ACCOUNT        = 60201,
		ERR_TOKEN_EXPIRED          = 60202,
		ERR_ACCOUNT_BANNED         = 60203,
		ERR_NOT_LOGGED_IN          = 60204,
		ERR_INVALID_TOKEN          = 60205,
		ERR_ATTR_NOT_SETTABLE      = 60206,
		ERR_ATTR_TOO_LONG          = 60207,
		ERR_NO_SUCH_ACCOUNT_NICK   = 60208,
	};
}

}

#endif
