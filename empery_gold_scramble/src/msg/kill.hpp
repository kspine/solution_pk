#ifndef EMPERY_GOLD_SCRAMBLE_MSG_KILL_HPP_
#define EMPERY_GOLD_SCRAMBLE_MSG_KILL_HPP_

#include <poseidon/cbpp/status_codes.hpp>

namespace EmperyGoldScramble {

namespace Msg {
	using namespace Poseidon::Cbpp::StatusCodes;

	enum {
		KILL_SESSION_GHOSTED                = -5001,
		KILL_OPERATOR_COMMAND               = -5002,
	};
}

}

#endif
