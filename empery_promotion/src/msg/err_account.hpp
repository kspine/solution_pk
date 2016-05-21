#ifndef EMPERY_PROMOTION_MSG_ERR_ACCOUNT_HPP_
#define EMPERY_PROMOTION_MSG_ERR_ACCOUNT_HPP_

#include <poseidon/cbpp/status_codes.hpp>

namespace EmperyPromotion {

namespace Msg {
	using namespace Poseidon::Cbpp::StatusCodes;

	enum {
		ERR_NO_SUCH_REFERRER                = 80101,
		ERR_INVALID_DEAL_PASSWORD           = 80102,
		ERR_NO_ENOUGH_ACCOUNT_BALANCE       = 80103,
		ERR_NO_SUCH_ACCOUNT                 = 80104,
		ERR_INVALID_PASSWORD                = 80105,
		ERR_ACCOUNT_BANNED                  = 80106,
		ERR_UNKNOWN_ACCOUNT_LEVEL           = 80107,
		ERR_DUPLICATE_LOGIN_NAME            = 80108,
		ERR_TRANSFER_DEST_NOT_FOUND         = 80109,
		ERR_TRANSFER_DEST_BANNED            = 80110,
		ERR_NO_SUCH_PAYER                   = 80111,
		ERR_NO_SUCH_BILL                    = 80112,
		ERR_DUPLICATE_PHONE_NUMBER          = 80113,
		ERR_BILL_CANCELLED                  = 80114,
		ERR_BILL_SETTLED                    = 80115,
		ERR_BILL_AMOUNT_MISMATCH            = 80116,
		ERR_ACCOUNT_DEACTIVATED             = 80117,
		ERR_NOT_SUBORDINATE                 = 80118,
		ERR_NO_ENOUGH_ITEMS                 = 80119,
		ERR_NO_MORE_ACCELERATION_CARDS      = 80120,
		ERR_ZERO_CARD_COUNT                 = 80121,
		ERR_ACCOUNT_LEVEL_NOT_FOR_SALE      = 80122,
		ERR_AUCTION_CENTER_ENABLED          = 80123,
		ERR_SHARED_RECHARGE_NOT_FOUND       = 80124,
		ERR_SHARED_RECHARGE_IN_PROGRESS     = 80125,
		ERR_SHARED_RECHARGE_NOT_FORMED      = 80126,
		ERR_SHARED_RECHARGE_NOT_REQUESTING  = 80127,
		ERR_WITHDRAWAL_PENDING              = 80128,

		ERR_W_D_SLIP_NOT_FOUND              = 80201,
		ERR_W_D_SLIP_CANCELLED              = 80202,
		ERR_W_D_SLIP_SETTLED                = 80203,
		ERR_NO_ENOUGH_W_D_BALANCE           = 80204,
		ERR_W_D_BALANCE_MISMATCH            = 80205,
	};
}

}

#endif
