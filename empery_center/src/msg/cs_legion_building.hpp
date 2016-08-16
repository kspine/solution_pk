#ifndef EMPERY_CENTER_MSG_CS_LEGION_HPP_
#define EMPERY_CENTER_MSG_CS_LEGION_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    CS_GetLegionBuildingInfoMessage
#define MESSAGE_ID      1630
#define MESSAGE_FIELDS  \

#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_CreateLegionBuildingMessage
#define MESSAGE_ID      1631
#define MESSAGE_FIELDS  \
	FIELD_VUINT          (ntype)
#include <poseidon/cbpp/message_generator.hpp>

}



}

#endif
