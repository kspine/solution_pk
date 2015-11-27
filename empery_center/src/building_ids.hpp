#ifndef EMPERY_CENTER_BUILDING_IDS_HPP_
#define EMPERY_CENTER_BUILDING_IDS_HPP_

#include "id_types.hpp"

namespace EmperyCenter {

namespace BuildingIds {

#define DEF_(name_, number_)        constexpr BuildingId name_(number_)

DEF_(ID_PRIMARY,                    1901001); // 领主府
DEF_(ID_STABLE,                     1902001); // 马厩
DEF_(ID_BARRACKS,                   1902002); // 兵营
DEF_(ID_ARCHER_BARRACKS,            1902003); // 弓兵营
DEF_(ID_WEAPONRY,                   1902004); // 器械营
DEF_(ID_ACADEMY,                    1903001); // 学院
DEF_(ID_WATCHTOWER,                 1905001); // 瞭望塔
DEF_(ID_WAREHOUSE,                  1906001); // 仓库
DEF_(ID_CITADEL_WALL,               1907001); // 城墙
DEF_(ID_DEFENSE_TOWER_1,            1913001); // 箭塔
DEF_(ID_DEFENSE_TOWER_2,            1913002); // 箭塔
DEF_(ID_CIVILIAN_1,                 1904001); // 民房
DEF_(ID_CIVILIAN_2,                 1904002); // 民房
DEF_(ID_CIVILIAN_3,                 1904003); // 民房
DEF_(ID_CIVILIAN_4,                 1904004); // 民房
DEF_(ID_CIVILIAN_5,                 1904005); // 民房

#undef DEF_

}

}

#endif
