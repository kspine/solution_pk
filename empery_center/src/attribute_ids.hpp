#ifndef EMPERY_CENTER_ATTRIBUTE_IDS_HPP_
#define EMPERY_CENTER_ATTRIBUTE_IDS_HPP_

#include "id_types.hpp"

namespace EmperyCenter {

namespace AttributeIds {

#define DEF_(name_, number_)	constexpr AttributeId name_(number_)

DEF_(ID_COORD_X,                      1);
DEF_(ID_COORD_Y,                      2);
DEF_(ID_HIT_POINTS,                   3);

DEF_(ID_LEADERSHIP,             2501001);

#undef DEF_

};

}

#endif
