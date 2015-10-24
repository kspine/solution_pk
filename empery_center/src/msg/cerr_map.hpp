#ifndef EMPERY_CENTER_MSG_CERR_MAP_HPP_
#define EMPERY_CENTER_MSG_CERR_MAP_HPP_

#include <poseidon/cbpp/status_codes.hpp>

namespace EmperyCenter {

namespace Msg {
	using namespace Poseidon::Cbpp::StatusCodes;

	enum {
		CERR_BROKEN_PATH            = 71301,
		CERR_MAP_RANGE_TOO_LARGE    = 71302,
		CERR_NO_SUCH_OBJECT         = 71303,
		CERR_NOT_YOUR_OBJECT        = 71304,
		CERR_NOT_MOVABLE_OBJECT     = 71305,
	};
}

}

#endif
