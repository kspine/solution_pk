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
	FIELD_VUINT         (mission_time_begin)	\
	FIELD_VUINT         (mission_time_remaining)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_CastleResource
#define MESSAGE_ID      498
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (resource_id)	\
	FIELD_VUINT         (count)
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
	FIELD_VUINT         (mission_time_begin)	\
	FIELD_VUINT         (mission_time_remaining)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
