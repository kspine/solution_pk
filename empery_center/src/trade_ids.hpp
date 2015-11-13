#ifndef EMPERY_CENTER_TRADE_IDS_HPP_
#define EMPERY_CENTER_TRADE_IDS_HPP_

#include "id_types.hpp"

namespace EmperyCenter {

namespace TradeIds {

#define DEF_(name_, number_)        constexpr TradeId name_(number_)

DEF_(ID_CHIPS,                      10001);

#undef DEF_

}

}

#endif
