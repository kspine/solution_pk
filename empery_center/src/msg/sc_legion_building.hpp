#ifndef EMPERY_CENTER_MSG_SC_LEGION_HPP_
#define EMPERY_CENTER_MSG_SC_LEGION_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    SC_LegionBuildings
#define MESSAGE_ID      1662
#define MESSAGE_FIELDS  \
	FIELD_ARRAY         (buildings,	\
		FIELD_STRING        (legion_building_uuid)	\
		FIELD_STRING        (legion_uuid)	\
		FIELD_STRING        (map_object_uuid)	\
		FIELD_VINT          (x)	\
		FIELD_VINT          (y)	\
		FIELD_VUINT         (building_level)	\
		FIELD_VUINT         (mission)	\
		FIELD_VUINT         (mission_duration)	\
		FIELD_VUINT         (mission_time_remaining)	\
		FIELD_VUINT         (output_type)	\
		FIELD_VUINT         (output_amount)	\
		FIELD_VUINT         (cd_time_remaining)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
