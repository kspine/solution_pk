#ifndef EMPERY_CENTER_REASON_IDS_HPP_
#define EMPERY_CENTER_REASON_IDS_HPP_

#include "id_types.hpp"

namespace EmperyCenter {

namespace ReasonIds {

constexpr ReasonId
	                                             // param1              param2              param3
	ID_ADMIN_OPERATION               ( 671001 ), // 自定义              自定义              自定义
	ID_UPGRADE_BUILDING              ( 672001 ), // 建筑 ID             等级                0
	ID_CANCEL_UPGRADE_BUILDING       ( 672002 ), // 建筑 ID             等级                0
	ID_UPGRADE_TECH                  ( 672003 ), // 科技 ID             等级                0
	ID_CANCEL_UPGRADE_TECH           ( 672004 ), // 科技 ID             等级                0
	ID_HARVEST_MAP_CELL              ( 672005 ), // 世界坐标 X          世界坐标 Y          土地购买券 ID
//	ID_HARVEST_OVERLAY               ( 672006 ), // 世界坐标 X          世界坐标 Y          覆盖图 ID
	ID_SPEED_UP_BUILDING_UPGRADE     ( 672007 ), // 建筑 ID             等级                0
	ID_SPEED_UP_TECH_UPGRADE         ( 672008 ), // 科技 ID             等级                0
	ID_UNPACK_INTO_CASTLE            ( 672009 ), // 城堡 UUID 高位      道具 ID             重复次数
	ID_BUY_ACCELERATION_CARD         ( 672010 ), // 玩家等级            0                   0
	ID_POPULATION_PRODUCTION         ( 672011 ), // 时间区间（毫秒）    0                   0
	ID_ENABLE_BATTALION              ( 672012 ), // 部队 ID             0                   0
	ID_PRODUCE_BATTALION             ( 672013 ), // 部队 ID             数量                0
	ID_CANCEL_PRODUCE_BATTALION      ( 672014 ), // 部队 ID             数量                0
	ID_HARVEST_BATTALION             ( 672015 ), // 部队 ID             数量                0
	ID_SPEED_UP_BATTALION_PRODUCTION ( 672016 ), // 部队 ID             数量                0
	ID_CREATE_BATTALION              ( 672017 ), // 城堡 UUID 高位      部队 UUID 高位      0
	ID_DISMISS_BATTALION             ( 672018 ), // 城堡 UUID 高位      部队 UUID 高位      0
	ID_REFILL_BATTALION              ( 672019 ), // 城堡 UUID 高位      部队 UUID 高位      0
	ID_POPULATION_CONSUMPTION        ( 672020 ), // 时间区间（毫秒）    0                   0
	ID_INIT_RESOURCES                ( 672021 ), // 初始数量            0                   0
//	ID_HARVEST_STRATEGIC_RESOURCE    ( 672022 ), // 世界坐标 X          世界坐标 Y          战略资源创建时间
	ID_DISSOLVE_BATTALION            ( 672023 ), // 0                   0                   0
	ID_BATTALION_UNLOAD              ( 672024 ), // 部队 UUID 高位      0                   0
	ID_SOLDIER_WOUNDED               ( 672025 ), // 部队 UUID 高位      部队受到伤害        伤兵率加成 * 1000
	ID_BEGIN_SOLDIER_TREATMENT       ( 672026 ), // 0                   0                   0
	ID_SPEED_UP_SOLDIER_TREATMENT    ( 672027 ), // 0                   0                   0
	ID_CASTLE_CAPTURED               ( 672028 ), // 0                   0                   0
	ID_BATTALION_UNLOAD_CRATE        ( 672029 ), // 部队 UUID 高位      0                   0
	ID_SOLDIER_HEALED                ( 672030 ), // 0                   0                   0

	ID_INIT_ITEMS                    ( 673001 ), // 初始数量            0                   0
	ID_AUTO_INCREMENT                ( 673002 ), // 自增长类型          自增长偏移          0
	ID_TRADE_REQUEST                 ( 673003 ), // 客户端请求 ID       交易 ID             重复次数
	ID_MAIL_ATTACHMENTS              ( 673004 ), // 邮件 UUID 高位      语言 ID             邮件类型
	ID_PAYMENT                       ( 673005 ), // 0                   0                   0
	ID_AUCTION_TRANSFER_LOCK         ( 673006 ), // 0                   0                   0
	ID_AUCTION_TRANSFER_COMMIT       ( 673007 ), // 0                   0                   0
	ID_AUCTION_TRANSFER_UNLOCK       ( 673008 ), // 0                   0                   0
	ID_AUCTION_CENTER_WITHDRAW       ( 673009 ), // 0                   0                   0
	ID_TASK_REWARD                   ( 673010 ), // 任务 ID             0                   0
	ID_LEVEL_BONUS                   ( 673011 ), // 纳税账号 UUID 高位  税收等级            0
	ID_INCOME_TAX                    ( 673012 ), // 纳税账号 UUID 高位  税收等级            0
	ID_LEVEL_BONUS_EXTRA             ( 673013 ), // 纳税账号 UUID 高位  税收等级            0
	ID_MONSTER_REWARD                ( 673014 ), // 怪物兵种 ID         掉落表内的 ID       0
	ID_MONSTER_REWARD_EXTRA          ( 673015 ), // 怪物兵种 ID         额外掉落表内的 ID   0
	ID_MONSTER_REWARD_COUNT          ( 673016 ), // 怪物兵种 ID         0                   0
	ID_UNPACK                        ( 673017 ), // 0                   道具 ID             重复次数
	ID_AUCTION_CENTER_BUY            ( 673018 ), // 0                   0                   0
	ID_CREATE_LEGION                 ( 673019 ), //创建军团 0                   道具 ID             扣除数量
	ID_DOANTE_LEGION                 ( 673020 ), // 军团捐献0                   道具 ID             数量
	ID_LEGION_STORE_EXCHANGE         ( 673021 ), // 军团商店兑换 0               道具 ID             数量
	ID_CREATE_LEAGUE                 ( 673022 ), //创建联盟 0                    道具 ID             扣除数量
	ID_CREATE_LEAGUE_FAIL            ( 673023 ), //创建联盟失败 0                道具 ID             扣除数量
	ID_EXPAND_LEAGUE                 ( 673024 ), //扩展联盟 0                    道具 ID             扣除数量

	ID_MAP_CELL_PURCHASE             ( 674001 ), // 世界坐标 X          世界坐标 Y          0
	ID_MAP_CELL_UPGRADE              ( 674002 ), // 世界坐标 X          世界坐标 Y          0
	ID_MAP_CELL_RECYCLE              ( 674003 ), // 世界坐标 X          世界坐标 Y          0
	ID_APPLY_ACCELERATION_CARD       ( 674004 ), // 世界坐标 X          世界坐标 Y          土地购买券 ID
	ID_RELOCATE_CASTLE               ( 674005 ), // 世界坐标 X          世界坐标 Y          0
	ID_CASTLE_BUFF                   ( 674006 ), // Buff ID             0                   0
	ID_HANG_UP_CASTLE                ( 674007 ), // 部队 UUID 高位      0                   0
	ID_CASTLE_PROTECTION             ( 674008 ), // 部队 UUID 高位      城堡等级            保护时间（毫秒）
	ID_CASTLE_PROTECTION_REFUND      ( 674009 ), // 部队 UUID 高位      城堡等级            保护时间（毫秒）
	ID_RETURN_OCCUPIED_MAP_CELL      ( 674010 ), // 世界坐标 X          世界坐标 Y          土地购买券 ID
	ID_OCCUPATION_END_RELOCATED      ( 674011 ), // 世界坐标 X          世界坐标 Y          0
	ID_CREATE_DUNGEON                ( 674012 ), // 副本 ID             0                   0
	ID_DUNGEON_MONSTER_REWARD        ( 674013 ), // 副本 ID             掉落表内的 ID       0
	ID_DUNGEON_TASK_REWARD           ( 674014 ), // 副本 ID             星级                0
	                                             //                     1 无星  7 一星
	                                             //                     8 二星  9 三星
	ID_HORN_MESSAGE                  ( 674015 ), // 0                   0                   0
	ID_DUNGEON_RESUSCITATION         ( 674016 ), // 部队 UUID 高位      部队受到伤害        复活率 * 1000
	ID_FINISH_DUNGEON                ( 674017 ), // 副本类型 ID         掉落表内的 ID       0
	ID_FINISH_DUNGEON_TASK           ( 674018 ), // 副本类型 ID         副本任务 ID         0
	ID_SOLDIER_KILL_ACCUMULATE       ( 674019 ), // 满足累计战力        原来累计战力        新累计战力
	ID_CASTLE_DMAGE_ACCUMULATE       ( 674020 ), // 满足伤害            原来累计伤害         新累计伤害
	ID_SOLDIER_KILL_RANK             ( 674021 ), // 活动id              排名                活动开始时间
	ID_WORLD_ACTIVITY                ( 674022 ), // 活动id              国家坐标x           国家坐标y
	ID_WORLD_ACTIVITY_RANK           ( 674023 ), // 活动id              排名                活动开始时间
	ID_LEGION_PACKAGE_TASK_PACKAGE_ITEM  (674024),//军团礼包 任务礼包 物品
	ID_LEGION_PACKAGE_SHARE_PACKAGE_ITEM (674025),//军团礼包 分享礼包 物品
	ID_LEGION_PACKAGE_SHARE_PACKAGE_COUNT_ITEM (674026),//军团礼包 分享礼包 已领计数物品
	ID_KING_CHAT                     (674027),   //0

	ID_LEGION_TASK_STAGE_REWARD      ( 675001 ),//  活动id               活动阶段           0
	ID_LEGION_TASK_PROCESS_REWARD    ( 675002 ),//  活动id               活动进度变化       0
	ID_LEGION_CREATE_BUILDING        ( 675003 ),//  建筑类型 ID          0                  0
	ID_LEGION_OPEN_GRUBE             ( 675004 ),//   0                   0                  0
	ID_LEGION_UPGRADE                ( 675005 ),//  军团UUID 高位        等级               0
	ID_LEGION_REPAIR_GRUBE           ( 675006 ),//  矿井UUID 高位        0                  0
	ID_LEGION_USE_DONATE_ITEM        ( 675007 ),//  道具 ID              道具数量           0
	ID_LEGION_EXCHANGE_ITEM          ( 675008 );//  0                    0                  0
}

}

#endif
