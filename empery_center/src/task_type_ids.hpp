#ifndef EMPERY_CENTER_TASK_TYPE_IDS_HPP_
#define EMPERY_CENTER_TASK_TYPE_IDS_HPP_

#include "id_types.hpp"

namespace EmperyCenter {

namespace TaskTypeIds {

constexpr TaskTypeId
	                                       // key       count       param1          param2          param3
	ID_UPGRADE_BUILDING_TO_LEVEL    (  1), // 建筑 ID   建筑数      建筑等级        -               -
	ID_CONSUME_RESOURCE             (  2), // 资源 ID   资源数      -               -               -
	ID_HARVEST_RESOURCE             (  3), // 资源 ID   资源数      -               -               -
	ID_HARVEST_BATTALION            (  4); // 部队 ID   部队数      -               -               -

}

}

#endif
