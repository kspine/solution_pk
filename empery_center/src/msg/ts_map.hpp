#ifndef EMPERY_CENTER_MSG_TS_MAP_HPP_
#define EMPERY_CENTER_MSG_TS_MAP_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    TS_MapInvalidateCastle
#define MESSAGE_ID      20399
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)   \
	FIELD_VINT          (coord_x)   \
	FIELD_VINT          (coord_y)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    TS_MapRemoveMapObject
#define MESSAGE_ID      20398
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
