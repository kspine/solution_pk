#ifndef EMPERY_CENTER_REASON_IDS_HPP_
#define EMPERY_CENTER_REASON_IDS_HPP_

#include "id_types.hpp"

namespace EmperyCenter {

namespace ReasonIds {

#define DEF_(name_, number_)        constexpr ReasonId name_(number_)

                                             // param1              param2              param3
DEF_(ID_ADMIN_OPERATION,            671001); // 自定义              自定义              自定义

DEF_(ID_UPGRADE_BUILDING,           672001); // 建筑 ID             等级                0
DEF_(ID_CANCEL_UPGRADE_BUILDING,    672002); // 建筑 ID             等级                0
DEF_(ID_UPGRADE_TECH,               672003); // 科技 ID             等级                0
DEF_(ID_CANCEL_UPGRADE_TECH,        672004); // 科技 ID             等级                0
DEF_(ID_HARVEST,                    672005); // 世界坐标 X          世界坐标 Y          土地购买券 ID

DEF_(ID_INIT_ITEMS,                 673001); // 初始数量            0                   0
DEF_(ID_AUTO_INCREMENT,             673002); // 自增长类型          自增长偏移          0
DEF_(ID_TRADE_REQUEST,              673003); // 客户端请求 ID       交易 ID             重复次数
DEF_(ID_MAIL_ATTACHMENTS,           673004); // 邮件 UUID 最后      语言 ID             0
                                             // 64 位（大端序）

DEF_(ID_MAP_CELL_PURCHASE,          674001); // 世界坐标 X          世界坐标 Y          0
DEF_(ID_MAP_CELL_UPGRADE,           674002); // 世界坐标 X          世界坐标 Y          旧的等级
DEF_(ID_MAP_CELL_RECYCLE,           674003); // 世界坐标 X          世界坐标 Y          0

#undef DEF_

}

}

#endif
