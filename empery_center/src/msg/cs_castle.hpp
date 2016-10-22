#ifndef EMPERY_CENTER_MSG_CS_CASTLE_HPP_
#define EMPERY_CENTER_MSG_CS_CASTLE_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    CS_CastleQueryInfo
#define MESSAGE_ID      400
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_CastleCreateBuilding
#define MESSAGE_ID      401
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (building_base_id)	\
	FIELD_VUINT         (building_id)
#include <poseidon/cbpp/message_generator.hpp>
/*
#define MESSAGE_NAME    CS_CastleCancelBuildingMission
#define MESSAGE_ID      402
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (building_base_id)
#include <poseidon/cbpp/message_generator.hpp>
*/
#define MESSAGE_NAME    CS_CastleUpgradeBuilding
#define MESSAGE_ID      403
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (building_base_id)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_CastleDestroyBuilding
#define MESSAGE_ID      404
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (building_base_id)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_CastleCompleteBuildingImmediately
#define MESSAGE_ID      405
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (building_base_id)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_CastleQueryIndividualBuildingInfo
#define MESSAGE_ID      406
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (building_base_id)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_CastleUpgradeTech
#define MESSAGE_ID      407
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (tech_id)
#include <poseidon/cbpp/message_generator.hpp>
/*
#define MESSAGE_NAME    CS_CastleCancelTechMission
#define MESSAGE_ID      408
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (tech_id)
#include <poseidon/cbpp/message_generator.hpp>
*/
#define MESSAGE_NAME    CS_CastleCompleteTechImmediately
#define MESSAGE_ID      409
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (tech_id)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_CastleQueryIndividualTechInfo
#define MESSAGE_ID      410
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (tech_id)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_CastleQueryMyCastleList
#define MESSAGE_ID      411
#define MESSAGE_FIELDS  \
	//
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_CastleHarvestAllResources
#define MESSAGE_ID      412
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_CastleQueryMapCells
#define MESSAGE_ID      413
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_CastleCreateImmigrants
#define MESSAGE_ID      414
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_CastleSpeedUpBuildingUpgrade
#define MESSAGE_ID      415
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (building_base_id)	\
	FIELD_VUINT         (item_id)	\
	FIELD_VUINT         (count)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_CastleSpeedUpTechUpgrade
#define MESSAGE_ID      416
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (tech_id)	\
	FIELD_VUINT         (item_id)	\
	FIELD_VUINT         (count)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_CastleUseResourceBox
#define MESSAGE_ID      417
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (item_id)	\
	FIELD_VUINT         (repeat_count)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_CastleBeginSoldierProduction
#define MESSAGE_ID      418
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (building_base_id)	\
	FIELD_VUINT         (map_object_type_id)	\
	FIELD_VUINT         (count)
#include <poseidon/cbpp/message_generator.hpp>
/*
#define MESSAGE_NAME    CS_CastleCancelSoldierProduction
#define MESSAGE_ID      419
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (building_base_id)
#include <poseidon/cbpp/message_generator.hpp>
*/
#define MESSAGE_NAME    CS_CastleHarvestSoldier
#define MESSAGE_ID      420
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (building_base_id)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_CastleSpeedUpSoldierProduction
#define MESSAGE_ID      421
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (building_base_id)	\
	FIELD_VUINT         (item_id)	\
	FIELD_VUINT         (count)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_CastleEnableSoldier
#define MESSAGE_ID      422
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (map_object_type_id)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_CastleCreateBattalion
#define MESSAGE_ID      423
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (map_object_type_id)	\
	FIELD_VUINT         (count)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_CastleCompleteSoldierProductionImmediately
#define MESSAGE_ID      424
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (building_base_id)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_CastleCreateChildCastle
#define MESSAGE_ID      425
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VINT          (x)	\
	FIELD_VINT          (y)	\
	FIELD_STRING        (name)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_CastleDissolveBattalion
#define MESSAGE_ID      426
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (map_object_type_id)	\
	FIELD_VUINT         (count)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_CastleUseSoldierBox
#define MESSAGE_ID      427
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (item_id)	\
	FIELD_VUINT         (repeat_count)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_CastleBeginTreatment
#define MESSAGE_ID      428
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_ARRAY         (soldiers,	\
		FIELD_VUINT         (map_object_type_id)	\
		FIELD_VUINT         (count)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_CastleSpeedUpTreatment
#define MESSAGE_ID      429
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (item_id)	\
	FIELD_VUINT         (count)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_CastleCompleteTreatmentImmediately
#define MESSAGE_ID      430
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_CastleQueryTreatmentInfo
#define MESSAGE_ID      431
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_CastleRelocate
#define MESSAGE_ID      432
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VINT          (coord_x)	\
	FIELD_VINT          (coord_y)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_CastleActivateBuff
#define MESSAGE_ID      433
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (item_id)	\
	FIELD_VUINT         (count)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_CastleReactivateCastle
#define MESSAGE_ID      434
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VINT          (coord_x)	\
	FIELD_VINT          (coord_y)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_CastleInitiateProtection
#define MESSAGE_ID      435
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (days)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_CastleCancelProtection
#define MESSAGE_ID      436
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_CastleReactivateCastleRandom
#define MESSAGE_ID      437
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_CastleSetName
#define MESSAGE_ID      438
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_STRING        (name)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_CastleUnlockTechEra
#define MESSAGE_ID      439
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (tech_era)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_CastleUseResourceGift
#define MESSAGE_ID      440
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (item_id)	\
	FIELD_VUINT         (repeat_count)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_UsePersonalDoateItem
#define MESSAGE_ID      441
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (item_id)	\
	FIELD_VUINT         (repeat_count)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_CastleResourceBattalionUnloadReset
#define MESSAGE_ID      442
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_CastleNewGuideCreateSolider
#define MESSAGE_ID      443
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (map_object_type_id)	\
	FIELD_VUINT         (count)
#include <poseidon/cbpp/message_generator.hpp>
}

}

#endif
