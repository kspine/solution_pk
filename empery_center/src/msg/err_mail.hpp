#ifndef EMPERY_CENTER_MSG_ERR_MAIL_HPP_
#define EMPERY_CENTER_MSG_ERR_MAIL_HPP_

#include <poseidon/cbpp/status_codes.hpp>

namespace EmperyCenter {

namespace Msg {
	using namespace Poseidon::Cbpp::StatusCodes;

	enum {
		ERR_NO_SUCH_MAIL                    = 71601,
		ERR_NO_ATTACHMENTS                  = 71602,
		ERR_ATTACHMENTS_FETCHED             = 71603,
		ERR_MAIL_IS_UNREAD                  = 71604,
		ERR_MAIL_HAS_ATTACHMENTS            = 71605,
		ERR_NO_SUCH_LANGUAGE_ID             = 71606,
	};
}

}

#endif
