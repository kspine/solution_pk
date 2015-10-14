#ifndef TEXAS_GATE_WESTWALK_MSG_ERR_ACCOUNT_HPP_
#define TEXAS_GATE_WESTWALK_MSG_ERR_ACCOUNT_HPP_

#include <poseidon/cbpp/status_codes.hpp>

namespace TexasGateWestwalk {

namespace Msg {
	using namespace Poseidon::Cbpp::StatusCodes;

	enum {
		ERR_NO_SUCH_ACCOUNT             = 80101,
		ERR_PASSWORD_INCORRECT          = 80102,
		ERR_TOKEN_EXPIRED               = 80103,
		ERR_DUPLICATE_ACCOUNT           = 80104,
		ERR_ACCOUNT_FROZEN              = 80105,
		ERR_INVALID_CAPTCHA             = 80106,
		ERR_ACCOUNT_NAME_TOO_LONG       = 80107,
		ERR_OLD_TOKEN_INCORRECT         = 80108,
		ERR_ACCOUNT_BANNED              = 80109,
		ERR_DISPOSABLE_PASSWD_EXPIRED   = 80110,
		ERR_PASSWORD_RAGAIN_COOLDOWN    = 80111,
	};
}

}

#endif
