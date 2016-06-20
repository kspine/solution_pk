#ifndef EMPERY_CENTER_MSG_CS_ACTIVITY_HPP_
#define EMPERY_CENTER_MSG_CS_ACTIVITY_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    CS_MapActivityInfo
#define MESSAGE_ID      1400
#define MESSAGE_FIELDS  \
        //
#include <poseidon/cbpp/message_generator.hpp>
}

}

#endif
