#ifndef EMPERY_CENTER_MSG_CERR_ITEM_HPP_
#define EMPERY_CENTER_MSG_CERR_ITEM_HPP_

#include <poseidon/cbpp/status_codes.hpp>

namespace EmperyCenter {

namespace Msg {
	using namespace Poseidon::Cbpp::StatusCodes;

	enum {
		CERR_NO_SUCH_ITEM_ID                = 71501,
		CERR_NO_ENOUGH_ITEMS                = 71502,
		CERR_ZERO_REPEAT_TIMES              = 71503,
		CERR_NO_SUCH_RECHARGE_ID            = 71504,
		CERR_NO_SUCH_SHOP_ID                = 71505,
	};
}

}

#endif
