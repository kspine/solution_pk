#ifndef EMPERY_CENTER_MSG_ERR_FRIEND_HPP_
#define EMPERY_CENTER_MSG_ERR_FRIEND_HPP_

#include <poseidon/cbpp/status_codes.hpp>

namespace EmperyCenter {

namespace Msg {
	using namespace Poseidon::Cbpp::StatusCodes;

	enum {
		ERR_NO_SUCH_FRIEND                      = 71701,
		ERR_FRIEND_LIST_FULL                    = 71702,
		ERR_FRIEND_LIST_FULL_PEER               = 71703,
		ERR_FRIEND_REQUESTING_LIST_FULL         = 71704,
		ERR_FRIEND_REQUESTED_LIST_FULL_PEER     = 71705,
		ERR_FRIEND_REQUESTING                   = 71706,
		ERR_FRIEND_NOT_REQUESTING               = 71707,
		ERR_ALREADY_A_FRIEND                    = 71708,
		ERR_FRIEND_LIST_FULL_INTERNAL           = 71709,
		ERR_FRIEND_CMP_XCHG_FAILURE_INTERNAL    = 71710,
		ERR_FRIEND_REQUESTED                    = 71711,
		ERR_FRIEND_NOT_REQUESTED                = 71712,
		ERR_FRIEND_BLACKLISTED                  = 71713,
		ERR_FRIEND_OFFLINE                      = 71714,
		ERR_FRIEND_NOT_IN_BLACKLIST             = 71715,
		ERR_FRIEND_CANNOT_REQUESTING_SELF       = 71716,
		ERR_FRIEND_IN_BLACKLIST                 = 71717,
	};
}

}

#endif
