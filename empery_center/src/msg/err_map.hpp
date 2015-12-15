#ifndef EMPERY_CENTER_MSG_ERR_MAP_HPP_
#define EMPERY_CENTER_MSG_ERR_MAP_HPP_

#include <poseidon/cbpp/status_codes.hpp>

namespace EmperyCenter {

namespace Msg {
	using namespace Poseidon::Cbpp::StatusCodes;

	enum {
		ERR_BROKEN_PATH                     = 71301,
		ERR_MAP_OBJECT_COORD_MISMATCH       = 71302,
		ERR_NO_SUCH_MAP_OBJECT              = 71303,
		ERR_NOT_YOUR_MAP_OBJECT             = 71304,
		ERR_NOT_MOVABLE_MAP_OBJECT          = 71305,
		ERR_CLUSTER_CONNECTION_LOST         = 71306,
		ERR_NO_SUCH_MAP_OBJECT_ON_CLUSTER   = 71307,
		ERR_MAP_OBJECT_ON_ANOTHER_CLUSTER   = 71308,
		ERR_MAP_OBJECT_IS_NOT_A_CASTLE      = 71309,
		ERR_MAP_CELL_ALREADY_HAS_AN_OWNER   = 71310,
		ERR_MAP_CELL_IS_TOO_FAR_AWAY        = 71311,
		ERR_TOO_MANY_MAP_CELLS              = 71312,
		ERR_LAND_PURCHASE_TICKET_NOT_FOUND  = 71313,
		ERR_BAD_LAND_PURCHASE_TICKET_TYPE   = 71314,
		ERR_NO_LAND_PURCHASE_TICKET         = 71315,
		ERR_MAX_MAP_CELL_LEVEL_EXCEEDED     = 71316,
		ERR_NO_LAND_UPGRADE_TICKET          = 71317,
		ERR_RESOURCE_NOT_PRODUCIBLE         = 71318,
		ERR_NO_TICKET_ON_MAP_CELL           = 71319,
		ERR_NOT_YOUR_MAP_CELL               = 71320,
		ERR_MAP_OBJECT_IS_NOT_IMMIGRANTS    = 71321,
		ERR_CANNOT_DEPLOY_IMMIGRANTS_HERE   = 71322,
		ERR_TOO_CLOSE_TO_ANOTHER_CASTLE     = 71323,
		ERR_NOT_ON_THE_SAME_MAP_SERVER      = 71324,
		ERR_CANNOT_DEPLOY_ON_OVERLAY        = 71325,
		ERR_UNPURCHASABLE_WITH_OVERLAY      = 71326,
	};
}

}

#endif
