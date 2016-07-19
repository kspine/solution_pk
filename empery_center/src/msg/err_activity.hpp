#ifndef EMPERY_CENTER_MSG_ERR_ACTIVITY_HPP_
#define EMPERY_CENTER_MSG_ERR_ACTIVITY_HPP_

#include <poseidon/cbpp/status_codes.hpp>

namespace EmperyCenter {

namespace Msg {
	using namespace Poseidon::Cbpp::StatusCodes;

	enum {
		ERR_NO_MAP_ACTIVITY                     = 72301,
		ERR_NO_ACTIVITY_FINISH                  = 72302,
		ERR_NO_WORLD_ACTIVITY                   = 72303,
	};
}

}

#endif