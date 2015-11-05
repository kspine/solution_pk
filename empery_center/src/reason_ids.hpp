#ifndef EMPERY_CENTER_REASON_IDS_HPP_
#define EMPERY_CENTER_REASON_IDS_HPP_

#include "id_types.hpp"

namespace EmperyCenter {

namespace ReasonIds {

#define DEF_(name_, number_)        constexpr ReasonId name_(number_)

                                             // param1           param2          param3
DEF_(ID_ADMIN_OPERATION,            671001); // 自定义           自定义          自定义
DEF_(ID_UPGRADE_BUILDING,           671002); // 建筑 ID          等级            0
DEF_(ID_CANCEL_UPGRADE_BUILDING,    671003); // 建筑 ID          等级            0
DEF_(ID_UPGRADE_TECH,               671004); // 科技 ID          等级            0
DEF_(ID_CANCEL_UPGRADE_TECH,        671005); // 科技 ID          等级            0

#undef DEF_

}

}

#endif
