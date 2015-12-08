#ifndef EMPERY_CENTER_MSG_ERR_ANNOUNCEMENT_HPP_
#define EMPERY_CENTER_MSG_ERR_ANNOUNCEMENT_HPP_

#include <poseidon/cbpp/status_codes.hpp>

namespace EmperyCenter {

namespace Msg {
	using namespace Poseidon::Cbpp::StatusCodes;

	enum {
		ERR_NO_SUCH_ANNOUNCEMENT            = 71901,
	};
}

}

#endif
