#ifndef EMPERY_CENTER_BUILDING_TYPE_IDS_HPP_
#define EMPERY_CENTER_BUILDING_TYPE_IDS_HPP_

#include "id_types.hpp"

namespace EmperyCenter {

namespace BuildingTypeIds {

constexpr BuildingTypeId
	ID_PRIMARY              (  1 ), // 领主府
	//
	ID_ACADEMY              (  3 ), // 学院
	ID_CIVILIAN             (  4 ), // 民房
	ID_WARCHTOWER           (  5 ), // 瞭望塔
	ID_WAREHOUSE            (  6 ), // 仓库
	ID_CITADEL_WALL         (  7 ), // 城墙
	ID_DEFENSE_TOWER        (  8 ), // 箭塔
	ID_PARADE_GROUND        (  9 ), // 校场
	//
	ID_STABLES              ( 17 ), // 马厩
	ID_BARRACKS             ( 18 ), // 兵营
	ID_ARCHER_BARRACKS      ( 19 ), // 弓兵营
	ID_WEAPONRY             ( 20 ), // 器械营
	ID_BOOT_CAMP            ( 21 ), // 新兵营地
	ID_MEDICAL_TENT         ( 22 ), // 医疗帐篷
	ID_HARVEST_WORKSHOP     ( 23 ), // 采集工坊
	ID_WAR_WORKSHOP         ( 24 ), // 战争工坊
	ID_TREE                 ( 25 ), // 未解锁
	ID_FORGE                ( 30 ); // 铁匠铺

}

}

#endif
