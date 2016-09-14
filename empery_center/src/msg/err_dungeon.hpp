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
		ERR_DUNGEON_NO_BATTALIONS               = 72508,
		ERR_DUNGEON_TOO_MANY_BATTALIONS         = 72509,
		ERR_NO_SUCH_DUNGEON                     = 72510,
		ERR_NO_SUCH_DUNGEON_OBJECT              = 72511,
		ERR_DUNGEON_SERVER_CONFLICT             = 72512,
		ERR_NOT_IN_DUNGEON                      = 72513,
		ERR_NO_DUNGEON_SERVER_AVAILABLE         = 72514,
		ERR_DUNGEON_SERVER_CONNECTION_LOST      = 72515,
		ERR_NOT_YOUR_DUNGEON_OBJECT             = 72516,
		ERR_NOT_MOVABLE_DUNGEON_OBJECT          = 72517,
		ERR_NO_SUCH_DUNGEON_OBJECT_TYPE         = 71518,
		ERR_DUNGEON_WAIT_CONTEXT_INVALID        = 71519,
		ERR_FAIL_SEARCH_TARGE                   = 72520,
		ERR_DUNGEON_SUSPENDED                   = 71521,
		ERR_DUNGEON_NOT_SUSPENDED               = 72522,
	};
}

}

#endif
