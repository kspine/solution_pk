#ifndef EMPERY_PROMOTION_ITEM_IDS_HPP_
#define EMPERY_PROMOTION_ITEM_IDS_HPP_

#include "id_types.hpp"

namespace EmperyPromotion {

namespace ItemIds {

#define DEF_(name_, number_)            constexpr ItemId name_(number_)

DEF_(ID_ACCELERATION_CARDS,             10001);
DEF_(ID_ACCOUNT_BALANCE,                10002);
DEF_(ID_WITHDRAWN_BALANCE,              10003);
DEF_(ID_GOLD_COINS,                     10004);
DEF_(ID_SHARED_BALANCE,                 10005);
DEF_(ID_USD,                            10006);

DEF_(ID_BALANCE_RECHARGED_HISTORICAL,   20001);
DEF_(ID_BALANCE_WITHDRAWN_HISTORICAL,   20002);
DEF_(ID_BALANCE_BUY_CARDS_HISTORICAL,   20003);

#undef DEF_

};

}

#endif
