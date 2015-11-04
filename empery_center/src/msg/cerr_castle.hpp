#ifndef EMPERY_CENTER_MSG_CERR_CASTLE_HPP_
#define EMPERY_CENTER_MSG_CERR_CASTLE_HPP_

#include <poseidon/cbpp/status_codes.hpp>

namespace EmperyCenter {

namespace Msg {
	using namespace Poseidon::Cbpp::StatusCodes;

	enum {
		CERR_NO_SUCH_CASTLE                 = 71401,
		CERR_NOT_CASTLE_OWNER               = 71402,
		CERR_NO_SUCH_CASTLE_BASE            = 71403,
		CERR_ANOTHER_BUILDING_THERE         = 71404,
		CERR_BUILDING_NOT_ALLOWED           = 71405,
		CERR_NO_CASTLE_MISSION              = 71406,
		CERR_CASTLE_MISSION_CONFLICT        = 71407,
		CERR_CASTLE_NO_ENOUGH_RESOURCES     = 71408,
		CERR_CASTLE_UPGRADE_MAX             = 71409,
		CERR_NO_BUILDING_THERE              = 71410,
		CERR_BUILDING_NOT_DESTRUCTIBLE      = 71411,
	};
}

}

#endif
