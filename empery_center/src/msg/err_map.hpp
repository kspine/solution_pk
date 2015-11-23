#ifndef EMPERY_CENTER_MSG_ERR_MAP_HPP_
#define EMPERY_CENTER_MSG_ERR_MAP_HPP_

#include <poseidon/cbpp/status_codes.hpp>

namespace EmperyCenter {

namespace Msg {
	using namespace Poseidon::Cbpp::StatusCodes;

	enum {
		ERR_BROKEN_PATH                     = 71301,
		ERR_MAP_OBJECT_COORD_MISMATCH       = 71302,
		ERR_NO_SUCH_MAP_OBJECT              = 71303,
		ERR_NOT_YOUR_MAP_OBJECT             = 71304,
		ERR_NOT_MOVABLE_MAP_OBJECT          = 71305,
		ERR_CLUSTER_CONNECTION_LOST         = 71306,
		ERR_NO_SUCH_MAP_OBJECT_ON_CLUSTER   = 71307,
		ERR_MAP_OBJECT_ON_ANOTHER_CLUSTER   = 71308,
	};
}

}

#endif
