#ifndef EMPERY_CENTER_MSG_ERR_CASTLE_HPP_
#define EMPERY_CENTER_MSG_ERR_CASTLE_HPP_

#include <poseidon/cbpp/status_codes.hpp>

namespace EmperyCenter {

namespace Msg {
	using namespace Poseidon::Cbpp::StatusCodes;

	enum {
		ERR_NO_SUCH_CASTLE                      = 71401,
		ERR_NOT_CASTLE_OWNER                    = 71402,
		ERR_NO_SUCH_CASTLE_BASE                 = 71403,
		ERR_ANOTHER_BUILDING_THERE              = 71404,
		ERR_BUILDING_NOT_ALLOWED                = 71405,
		ERR_NO_BUILDING_MISSION                 = 71406,
		ERR_BUILDING_MISSION_CONFLICT           = 71407,
		ERR_CASTLE_NO_ENOUGH_RESOURCES          = 71408,
		ERR_BUILDING_UPGRADE_MAX                = 71409,
		ERR_NO_BUILDING_THERE                   = 71410,
		ERR_BUILDING_NOT_DESTRUCTIBLE           = 71411,
		ERR_PREREQUISITE_NOT_MET                = 71412,
		ERR_NO_TECH_MISSION                     = 71413,
		ERR_TECH_MISSION_CONFLICT               = 71414,
		ERR_TECH_UPGRADE_MAX                    = 71415,
		ERR_NO_SUCH_BUILDING                    = 71416,
		ERR_NO_SUCH_TECH                        = 71417,
		ERR_DISPLAY_PREREQUISITE_NOT_MET        = 71418,
		ERR_BUILD_LIMIT_EXCEEDED                = 71419,
		ERR_CASTLE_HAS_NO_MAP_CELL              = 71420,
		ERR_CASTLE_LEVEL_TOO_LOW                = 71421,
		ERR_MAX_NUMBER_OF_IMMIGRANT_GROUPS      = 71422,
		ERR_NO_ROOM_FOR_NEW_UNIT                = 71423,
		ERR_CASTLE_HAS_NO_CHILDREN              = 71424,
		ERR_NOT_BUILDING_UPGRADE_ITEM           = 71425,
		ERR_NOT_TECH_UPGRADE_ITEM               = 71426,
		ERR_NO_WAREHOUSE                        = 71427,
		ERR_WAREHOUSE_FULL                      = 71428,
		ERR_BUILDING_QUEUE_FULL                 = 71429,
		ERR_TECH_QUEUE_FULL                     = 71430,
		ERR_ACCOUNT_MAX_IMMIGRANT_GROUPS        = 71431,
		ERR_ACADEMY_UPGRADE_IN_PROGRESS         = 71432,
		ERR_TECH_UPGRADE_IN_PROGRESS            = 71433,
		ERR_NO_ACADEMY                          = 71434,
		ERR_BARRACKS_UPGRADE_IN_PROGRESS        = 71435,
		ERR_BATTALION_PRODUCTION_IN_PROGRESS    = 71436,
		ERR_BATTALION_PRODUCTION_CONFLICT       = 71437,
		ERR_NO_SUCH_MAP_OBJECT_TYPE             = 71438,
		ERR_NO_BATTALION_PRODUCTION             = 71439,
		ERR_BATTALION_UNAVAILABLE               = 71440,
		ERR_BATTALION_UNLOCKED                  = 71441,
		ERR_PREREQUISITE_BATTALION_NOT_MET      = 71442,
		ERR_BATTALION_PRODUCTION_COMPLETE       = 71443,
		ERR_BATTALION_PRODUCTION_INCOMPLETE     = 71444,
		ERR_NOT_BATTALION_PRODUCTION_ITEM       = 71445,
		ERR_FACTORY_ID_MISMATCH                 = 71446,
		ERR_ZERO_SOLDIER_COUNT                  = 71447,
		ERR_CASTLE_NO_ENOUGH_SOLDIERS           = 71448,
		ERR_TOO_MANY_SOLDIERS_FOR_BATTALION     = 71449,
		ERR_MAX_BATTALION_COUNT_EXCEEDED        = 71450,
		ERR_OPERATION_PREREQUISITE_NOT_MET      = 71451,
		ERR_MEDICAL_TENT_NOT_EMPTY              = 71452,
		ERR_MEDICAL_TENT_TREATMENT_IN_PROGRESS  = 71453,
		ERR_MEDICAL_TENT_UPGRADE_IN_PROGRESS    = 71454,
		ERR_MEDICAL_TENT_BUSY                   = 71455,
		ERR_NO_ENOUGH_WOUNDED_SOLDIERS          = 71456,
		ERR_NO_BATTALION_TREATMENT              = 71457,
		ERR_NOT_TREATMENT_ITEM                  = 71458,
		ERR_NO_MEDICAL_TENT                     = 71459,
		ERR_BATTLE_IN_PROGRESS                  = 71460,
		ERR_CASTLE_NOT_HUNG_UP                  = 71461,
		ERR_CASTLE_NOT_UNDER_PROTECTION         = 71462,
		ERR_BATTALION_UNDER_PROTECTION          = 71463,
		ERR_NO_START_POINTS_AVAILABLE           = 71464,
		ERR_DEFENSE_BUILDING_UPGRADE_IN_PROGESS = 71465,
		ERR_PROTECTION_IN_PROGRESS              = 71466,
	};
}

}

#endif
