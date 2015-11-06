#ifndef EMPERY_CENTER_MAP_OBJECT_TYPE_IDS_HPP_
#define EMPERY_CENTER_MAP_OBJECT_TYPE_IDS_HPP_

#include "id_types.hpp"

namespace EmperyCenter {

namespace MapObjectTypeIds {

#define DEF_(name_, number_)        constexpr MapObjectTypeId name_(number_)

DEF_(ID_CASTLE,                     123456);

#undef DEF_

}

}

#endif
