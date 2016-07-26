#ifndef EMPERY_CENTER_ACTIVITY_IDS_HPP_
#define EMPERY_CENTER_ACTIVITY_IDS_HPP_

#include "id_types.hpp"

namespace EmperyCenter {

namespace ActivityIds {

constexpr ActivityId
	ID_MAP_ACTIVITY                    ( 3500001 ),
	ID_WORLD_ACTIVITY                ( 3500002 );

constexpr MapActivityId
    ID_MAP_ACTIVITY_HARVEST            (3501001),
	ID_MAP_ACTIVITY_RESOURCE           (3502001),
	ID_MAP_ACTIVITY_MONSTER            (3503001),
	ID_MAP_ACTIVITY_GOBLIN             (3504001),
	ID_MAP_ACTIVITY_KILL_SOLDIER       (3505001),
	ID_MAP_ACTIVITY_CASTLE_DAMAGE      (3506001);

constexpr WorldActivityId
	ID_WORLD_ACTIVITY_MONSTER        (3510001),
	ID_WORLD_ACTIVITY_RESOURCE       (3511001),
	ID_WORLD_ACTIVITY_BOSS           (3512001);
}

}

#endif
