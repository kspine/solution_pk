#ifndef EMPERY_CENTER_MSG_KILL_HPP_
#define EMPERY_CENTER_MSG_KILL_HPP_

#include <poseidon/cbpp/status_codes.hpp>

namespace EmperyCenter {

namespace Msg {
	using namespace Poseidon::Cbpp::StatusCodes;

	enum {
		KILL_SESSION_GHOSTED                = -5001,
		KILL_OPERATOR_COMMAND               = -5002,
		KILL_SHUTDOWN                       = -5003,

		KILL_INVALID_NUMERICAL_COORD        = -6001,
		KILL_CLUSTER_SERVER_CONFLICT        = -6002,
		KILL_CLUSTER_SERVER_CONFLICT_GLOBAL = -6003,
		KILL_CLUSTER_SERVER_RESYNCHRONIZE   = -6004,
		KILL_DUNGEON_SERVER_RESYNCHRONIZE   = -6005,
	};
}

}

#endif
