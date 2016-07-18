#ifndef EMPERY_CENTER_MSG_DS_DUNGEON_HPP_
#define EMPERY_CENTER_MSG_DS_DUNGEON_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    DS_DungeonRegisterServer
#define MESSAGE_ID      50000
#define MESSAGE_FIELDS  \
	//
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
