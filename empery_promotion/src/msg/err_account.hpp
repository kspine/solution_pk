#ifndef EMPERY_PROMOTION_MSG_ERR_ACCOUNT_HPP_
#define EMPERY_PROMOTION_MSG_ERR_ACCOUNT_HPP_

#include <poseidon/cbpp/status_codes.hpp>

namespace EmperyPromotion {

namespace Msg {
	using namespace Poseidon::Cbpp::StatusCodes;

	enum {
		ERR_NO_SUCH_REFERRER            = 80101,
		ERR_INVALID_DEAL_PASSWORD       = 80102,
		ERR_NO_ENOUGH_ACCOUNT_BALANCE   = 80103,
		ERR_NO_SUCH_ACCOUNT             = 80104,
		ERR_INVALID_PASSWORD            = 80105,
		ERR_ACCOUNT_BANNED              = 80106,
		ERR_UNKNOWN_ACCOUNT_LEVEL       = 80107,
		ERR_DUPLICATE_LOGIN_NAME        = 80108,
		ERR_TRANSFER_DEST_NOT_FOUND     = 80109,
		ERR_TRANSFER_DEST_BANNED        = 80110,
		ERR_NO_SUCH_PAYER               = 80111,

		ERR_W_D_SLIP_NOT_FOUND          = 80201,
		ERR_W_D_SLIP_CANCELLED          = 80202,
		ERR_W_D_SLIP_SETTLED            = 80203,
		ERR_NO_ENOUGH_W_D_BALANCE       = 80204,
		ERR_W_D_BALANCE_MISMATCH        = 80205,
	};
}

}

#endif
