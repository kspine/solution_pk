#ifndef EMPERY_CENTER_MSG_KS_INIT_HPP_
#define EMPERY_CENTER_MSG_KS_INIT_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME	KS_InitRegisterCluster
#define MESSAGE_ID		32300
#define MESSAGE_FIELDS	\
	FIELD_VINT			(mapX)	\
	FIELD_VINT			(mapY)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
