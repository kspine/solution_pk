#ifndef EMPERY_CENTER_MSG_ST_MAP_HPP_
#define EMPERY_CENTER_MSG_ST_MAP_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    ST_MapInvalidateCastle
#define MESSAGE_ID      20300
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
