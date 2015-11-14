#ifndef EMPERY_CENTER_MSG_CERR_MAIL_HPP_
#define EMPERY_CENTER_MSG_CERR_MAIL_HPP_

#include <poseidon/cbpp/status_codes.hpp>

namespace EmperyCenter {

namespace Msg {
	using namespace Poseidon::Cbpp::StatusCodes;

	enum {
		CERR_NO_SUCH_MAIL                   = 71601,
		CERR_NO_ATTACHMENTS                 = 71602,
		CERR_ATTACHMENTS_FETCHED            = 71603,
		CERR_MAIL_IS_UNREAD                 = 71604,
		CERR_MAIL_HAS_ATTACHMENTS           = 71605,
		CERR_NO_SUCH_LANGUAGE_ID            = 71606,
	};
}

}

#endif
