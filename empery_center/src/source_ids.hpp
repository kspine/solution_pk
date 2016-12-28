#ifndef EMPERY_CENTER_SOURCE_IDS_HPP_
#define EMPERY_CENTER_SOURCE_IDS_HPP_

#include "id_types.hpp"

namespace EmperyCenter {

namespace SourceIds {

constexpr SourceId
	ID_COMMON                      ( 0 ),    //（数据库加载，无需特殊展示资源或者道具功能）
	ID_TASK_REWARD                 (100001), //任务奖励
	ID_HARVEST_TERRITORY           (100002), //领地收获
	ID_SIGN_REWARD                 (100003), //签到
	ID_ACTIVITY_REWARD             (100004), //活动相关
	ID_USE_ITEM                    (100005); //游戏中道具使用                    
}

}

#endif
