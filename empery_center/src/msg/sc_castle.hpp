#ifndef EMPERY_CENTER_MSG_SC_CASTLE_HPP_
#define EMPERY_CENTER_MSG_SC_CASTLE_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    SC_CastleBuildingBase
#define MESSAGE_ID      499
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (building_base_id)	\
	FIELD_VUINT         (building_id)	\
	FIELD_VUINT         (building_level)	\
	FIELD_VUINT         (mission)	\
	FIELD_VUINT         (mission_duration)	\
	FIELD_VUINT         (reserved)	\
	FIELD_VUINT         (reserved2)	\
	FIELD_VUINT         (mission_time_remaining)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_CastleResource
#define MESSAGE_ID      498
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (resource_id)	\
	FIELD_VUINT         (amount)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_CastleTech
#define MESSAGE_ID      497
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (tech_id)	\
	FIELD_VUINT         (tech_level)	\
	FIELD_VUINT         (mission)	\
	FIELD_VUINT         (mission_duration)	\
	FIELD_VUINT         (reserved)	\
	FIELD_VUINT         (reserved2)	\
	FIELD_VUINT         (mission_time_remaining)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_CastleSoldier
#define MESSAGE_ID      496
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (map_object_type_id)	\
	FIELD_VUINT         (count)	\
	FIELD_VUINT         (enabled)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_CastleSoldierProduction
#define MESSAGE_ID      495
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (building_base_id)	\
	FIELD_VUINT         (map_object_type_id)	\
	FIELD_VUINT         (count)	\
	FIELD_VUINT         (production_duration)	\
	FIELD_VUINT         (production_time_remaining)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_CastleWoundedSoldier
#define MESSAGE_ID      494
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (map_object_type_id)	\
	FIELD_VUINT         (count)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_CastleTreatment
#define MESSAGE_ID      493
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_ARRAY         (soldiers,	\
		FIELD_VUINT         (map_object_type_id)	\
		FIELD_VUINT         (count)	\
	)	\
	FIELD_VUINT         (treatment_duration)	\
	FIELD_VUINT         (treatment_time_remaining)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_CastleTechEra
#define MESSAGE_ID      492
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (tech_era)	\
	FIELD_VUINT         (unlocked)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_CastleResourceBattalionUnload
#define MESSAGE_ID      491
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (resource_id)	\
	FIELD_VUINT         (count)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_CastleOfflineUpgradeBuildingBase
#define MESSAGE_ID      480
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_STRING        (building_base_ids)
#include <poseidon/cbpp/message_generator.hpp>
}

}

#endif
