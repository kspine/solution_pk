#ifndef EMPERY_CENTER_REASON_IDS_HPP_
#define EMPERY_CENTER_REASON_IDS_HPP_

#include "id_types.hpp"

namespace EmperyCenter {

namespace ReasonIds {

constexpr ReasonId
	                                            // param1              param2              param3
	ID_ADMIN_OPERATION              ( 671001 ), // 自定义              自定义              自定义
	ID_UPGRADE_BUILDING             ( 672001 ), // 建筑 ID             等级                0
	ID_CANCEL_UPGRADE_BUILDING      ( 672002 ), // 建筑 ID             等级                0
	ID_UPGRADE_TECH                 ( 672003 ), // 科技 ID             等级                0
	ID_CANCEL_UPGRADE_TECH          ( 672004 ), // 科技 ID             等级                0
	ID_HARVEST_MAP_CELL             ( 672005 ), // 世界坐标 X          世界坐标 Y          土地购买券 ID
	ID_HARVEST_OVERLAY              ( 672006 ), // 世界坐标 X          世界坐标 Y          覆盖图 id
	ID_SPEED_UP_BUILDING_UPGRADE    ( 672007 ), // 建筑 ID             等级                0
	ID_SPEED_UP_TECH_UPGRADE        ( 672008 ), // 科技 ID             等级                0

	ID_INIT_ITEMS                   ( 673001 ), // 初始数量            0                   0
	ID_AUTO_INCREMENT               ( 673002 ), // 自增长类型          自增长偏移          0
	ID_TRADE_REQUEST                ( 673003 ), // 客户端请求 ID       交易 ID             重复次数
	ID_MAIL_ATTACHMENTS             ( 673004 ), // 邮件 UUID 最后      语言 ID             0
	                                            // 64 位（大端序）

	ID_MAP_CELL_PURCHASE            ( 674001 ), // 世界坐标 X          世界坐标 Y          0
	ID_MAP_CELL_UPGRADE             ( 674002 ), // 世界坐标 X          世界坐标 Y          0
	ID_MAP_CELL_RECYCLE             ( 674003 ), // 世界坐标 X          世界坐标 Y          0
	ID_APPLY_ACCELERATION_CARD      ( 674004 ); // 世界坐标 X          世界坐标 Y          土地购买券 ID

}

}

#endif
