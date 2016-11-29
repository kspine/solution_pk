#ifndef EMPERY_CENTER_MSG_SC_MAP_COUNTRY_HPP_
#define EMPERY_CENTER_MSG_SC_MAP_COUNTRY_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter{
 namespace Msg{
 #define MESSAGE_NAME    SC_GetMapCountryInfoReqMessage
 #define MESSAGE_ID      2250
 #define MESSAGE_FIELDS  \
             FIELD_STRING    (map_name)
 #include <poseidon/cbpp/message_generator.hpp>
}
}

#endif