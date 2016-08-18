#ifndef EMPERY_CENTER_TASK_TYPE_IDS_HPP_
#define EMPERY_CENTER_TASK_TYPE_IDS_HPP_

#include "id_types.hpp"

namespace EmperyCenter {

namespace TaskTypeIds {

constexpr TaskTypeId
	                                       // key       count       param1          param2
	ID_UPGRADE_BUILDING_TO_LEVEL    (  1), // 建筑 ID   建筑数      建筑等级        -
	ID_CONSUME_RESOURCES            (  2), // 资源 ID   资源数      -               -
	ID_HARVEST_RESOURCES            (  3), // 资源 ID   资源数      -               -
	ID_HARVEST_SOLDIERS             (  4), // 部队 ID   部队数      -               -
	ID_WIPE_OUT_MONSTERS            (  5), // 怪物 ID   单位数      -               -
	ID_WIPE_OUT_ENEMY_BATTALIONS    (  6), // 部队 ID   单位数      -               -
	ID_LEGION_DONATE    			(  12); // 军团捐献 单位数      -               -
}

}

#endif
