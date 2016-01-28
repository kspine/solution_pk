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

#define MESSAGE_NAME    CS_CastleCancelBuildingMission
#define MESSAGE_ID      402
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (building_base_id)
#include <poseidon/cbpp/message_generator.hpp>

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

#define MESSAGE_NAME    CS_CastleCancelTechMission
#define MESSAGE_ID      408
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (tech_id)
#include <poseidon/cbpp/message_generator.hpp>

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

#define MESSAGE_NAME    CS_CastleBeginBattalionProduction
#define MESSAGE_ID      418
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (building_base_id)	\
	FIELD_VUINT         (map_object_type_id)	\
	FIELD_VUINT         (count)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_CastleCancelBattalionProduction
#define MESSAGE_ID      419
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (building_base_id)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_CastleHarvestBattalion
#define MESSAGE_ID      420
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (building_base_id)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_CastleSpeedUpBattalionProduction
#define MESSAGE_ID      421
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (building_base_id)	\
	FIELD_VUINT         (item_id)	\
	FIELD_VUINT         (count)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_CastleEnableBattalion
#define MESSAGE_ID      422
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (map_object_type_id)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_CastleDismissBattalion
#define MESSAGE_ID      423
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (map_object_type_id)	\
	FIELD_VUINT         (count)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
