#ifndef EMPERY_CENTER_MSG_ERR_ITEM_HPP_
#define EMPERY_CENTER_MSG_ERR_ITEM_HPP_

#include <poseidon/cbpp/status_codes.hpp>

namespace EmperyCenter {

namespace Msg {
	using namespace Poseidon::Cbpp::StatusCodes;

	enum {
		ERR_NO_SUCH_ITEM_ID                     = 71501,
		ERR_NO_ENOUGH_ITEMS                     = 71502,
		ERR_ZERO_REPEAT_COUNT                   = 71503,
		ERR_NO_SUCH_RECHARGE_ID                 = 71504,
		ERR_NO_SUCH_SHOP_ID                     = 71505,
		ERR_ITEM_TYPE_MISMATCH                  = 71506,
		ERR_ITEM_NOT_USABLE                     = 71507,
		ERR_NO_SUCH_TRADE_ID                    = 71508,
		ERR_NOT_DUNGEON_TRADE                   = 71509,
	};
}

}

#endif
