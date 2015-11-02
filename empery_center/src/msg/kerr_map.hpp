#ifndef EMPERY_CENTER_MSG_KERR_MAP_HPP_
#define EMPERY_CENTER_MSG_KERR_MAP_HPP_

#include <poseidon/cbpp/status_codes.hpp>

namespace EmperyCenter {

namespace Msg {
	using namespace Poseidon::Cbpp::StatusCodes;

	enum {
		KERR_MAP_SERVER_CONFLICT            = 72301,
		KERR_VIEW_RANGE_TOO_LARGE           = 72302,
	};
}

}

#endif
