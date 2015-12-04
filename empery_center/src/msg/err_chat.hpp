#ifndef EMPERY_CENTER_MSG_ERR_CHAT_HPP_
#define EMPERY_CENTER_MSG_ERR_CHAT_HPP_

#include <poseidon/cbpp/status_codes.hpp>

namespace EmperyCenter {

namespace Msg {
	using namespace Poseidon::Cbpp::StatusCodes;

	enum {
		ERR_ACCOUNT_QUIETED                 = 71801,
		ERR_CHAT_FLOOD                      = 71802,
		ERR_INVALID_CHAT_MESSAGE_SLOT       = 71803,
		ERR_CANNOT_SEND_TO_SYSTEM_CHANNEL   = 71804,
		ERR_NO_SUCH_CHAT_MESSAGE            = 71805,
	};
}

}

#endif
