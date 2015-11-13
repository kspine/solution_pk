#ifndef EMPERY_CENTER_MSG_KS_MAP_HPP_
#define EMPERY_CENTER_MSG_KS_MAP_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    KS_MapRegisterCluster
#define MESSAGE_ID      32300
#define MESSAGE_FIELDS  \
	FIELD_VINT          (map_x)	\
	FIELD_VINT          (map_y)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
