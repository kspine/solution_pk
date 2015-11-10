#ifndef EMPERY_GOLD_SCRAMBLE_MSG_ERR_ACCOUNT_HPP_
#define EMPERY_GOLD_SCRAMBLE_MSG_ERR_ACCOUNT_HPP_

#include <poseidon/cbpp/status_codes.hpp>

namespace EmperyGoldScramble {

namespace Msg {
	using namespace Poseidon::Cbpp::StatusCodes;

	enum {
		ERR_INVALID_SESSION_ID              = 82201,
		ERR_NO_ENOUGH_GOLD_COINS            = 82202,
		ERR_NO_ENOUGH_ACCOUNT_BALANCE       = 82203,
		ERR_MULTIPLE_LOGIN                  = 82204,
		ERR_NO_GAME_IN_PROGRESS             = 82205,
	};
}

}

#endif
