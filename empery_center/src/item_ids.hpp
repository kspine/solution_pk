#ifndef EMPERY_CENTER_ITEM_IDS_HPP_
#define EMPERY_CENTER_ITEM_IDS_HPP_

#include "id_types.hpp"

namespace EmperyCenter {

namespace ItemIds {

#define DEF_(name_, number_)        constexpr ItemId name_(number_)

DEF_(ID_SIGNING_IN,                 2100020);

#undef DEF_

}

}

#endif
