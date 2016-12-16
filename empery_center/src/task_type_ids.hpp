#ifndef EMPERY_CENTER_TASK_TYPE_IDS_HPP_
#define EMPERY_CENTER_TASK_TYPE_IDS_HPP_

#include "id_types.hpp"

namespace EmperyCenter {
	namespace TaskTypeIds {
		constexpr TaskTypeId
			// key       count       param1          param2
			ID_UPGRADE_BUILDING_TO_LEVEL(1), // 建筑 ID   建筑数      建筑等级        -
			ID_CONSUME_RESOURCES(2), // 资源 ID   资源数      -               -
			ID_HARVEST_RESOURCES(3), // 资源 ID   资源数      -               -
			ID_HARVEST_SOLDIERS(4), // 部队 ID   部队数      -               -
			ID_WIPE_OUT_MONSTERS(5), // 怪物 ID   单位数      -               -
			ID_WIPE_OUT_ENEMY_BATTALIONS(6), // 部队 ID   单位数      -               -
			ID_HARVEST_STRATEGIC_RESOURCE(7), // 统计战略资源ID 数量
			ID_BUILD_SOLDIERS(8),   // 累计建造部队ID
			ID_DESTROY_MONSTERS(9), // 消灭野怪战斗力ID
			ID_DESTROY_SOLDIERS(10), //消灭士兵战斗力ID
			ID_CHARGE_DIAMOND(11), //充值钻石ID
			ID_LEGION_DONATE(12), // 军团捐献 单位数
			ID_DUNGEON_CLEARANCE(13),
			ID_WIPE_OUT_LEVEL_MONSTER(14),//怪物等级 怪物数量
			ID_JOIN_LEGION(15),//加入军团
			ID_HARVEST_TYPE_SOLIDERS(16),//arm_type  数量
			ID_HARVEST_SPECIFIC_STRATEGIC_RESOURCE(17),// 资源id 数量
			ID_ENTER_DUNGEON(18);//副本id 数量
	}

	namespace TaskPrimaryKeyIds{
		constexpr TaskPrimaryKeyId
			ID_JOIN_LEGION(5710007);
	}

	namespace TaskLegionPackageKeyIds {
		constexpr TaskLegionPackageKeyId
			ID_HARVEST_STRATEGIC_RESOURCE(5710001),
			ID_BUILD_SOLDIERS(5710002),
			ID_DESTROY_MONSTERS(5710003),
			ID_DESTROY_SOLDIERS(5710004),
			ID_CHARGE_DIAMOND(5710005);
	}
	namespace TaskLegionKeyIds {
		constexpr TaskLegionKeyId
			ID_HARVEST_STRATEGIC_RESOURCE(5710001),
			ID_BUILD_SOLDIERS(5710002),
			ID_WIPE_OUT_MONSTERS(5710003),
			ID_WIPE_OUT_SOLIDERS(5710004),
			ID_LEGION_DONATE(5710006);
	}
}

#endif