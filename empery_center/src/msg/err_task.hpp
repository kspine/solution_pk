#ifndef EMPERY_CENTER_MSG_ERR_TASK_HPP_
#define EMPERY_CENTER_MSG_ERR_TASK_HPP_

#include <poseidon/cbpp/status_codes.hpp>

namespace EmperyCenter {

namespace Msg {
	using namespace Poseidon::Cbpp::StatusCodes;

	enum {
		ERR_NO_SUCH_TASK                        = 72201,
		ERR_TASK_NOT_ACCOMPLISHED               = 72202,
		ERR_TASK_REWARD_FETCHED                 = 72203,
	};
}

}

#endif
