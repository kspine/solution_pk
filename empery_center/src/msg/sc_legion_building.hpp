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
		FIELD_STRING        (type)	\
		FIELD_STRING        (level)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
