#ifndef EMPERY_CENTER_MSG_ERR_DUNGEON_HPP_
#define EMPERY_CENTER_MSG_ERR_DUNGEON_HPP_

#include <poseidon/cbpp/status_codes.hpp>

namespace EmperyCenter {

namespace Msg {
	using namespace Poseidon::Cbpp::StatusCodes;

	enum {
		ERR_NO_SUCH_DUNGEON_ID                  = 72501,
		ERR_DUNGEON_PREREQUISITE_NOT_MET        = 72502,
		ERR_DUNGEON_DISPOSED                    = 72503,
		ERR_DUNGEON_TOO_FEW_SOLDIERS            = 72504,
		ERR_DUNGEON_TOO_MANY_SOLDIERS           = 72505,
		ERR_BATTALION_TYPE_FORBIDDEN            = 72506,
		ERR_BATTALION_NOT_IDLE                  = 72507,
	};
}

}

#endif
