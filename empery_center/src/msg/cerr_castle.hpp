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
		CERR_NO_BUILDING_MISSION            = 71406,
		CERR_BUILDING_MISSION_CONFLICT      = 71407,
		CERR_CASTLE_NO_ENOUGH_RESOURCES     = 71408,
		CERR_BUILDING_UPGRADE_MAX           = 71409,
		CERR_NO_BUILDING_THERE              = 71410,
		CERR_BUILDING_NOT_DESTRUCTIBLE      = 71411,
		CERR_PREREQUISITE_NOT_MET           = 71412,
		CERR_NO_TECH_MISSION                = 71413,
		CERR_TECH_MISSION_CONFLICT          = 71414,
		CERR_TECH_UPGRADE_MAX               = 71415,
		CERR_NO_SUCH_BUILDING               = 71416,
		CERR_NO_SUCH_TECH                   = 71417,
		CERR_DISPLAY_PREREQUISITE_NOT_MET   = 71418,
		CERR_BUILDING_EXISTS                = 71419,
	};
}

}

#endif
