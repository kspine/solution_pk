#ifndef EMPERY_CENTER_ACTIVITY_IDS_HPP_
#define EMPERY_CENTER_ACTIVITY_IDS_HPP_

#include "id_types.hpp"

namespace EmperyCenter {

namespace ActivityIds {

constexpr ActivityId
	ID_MAP_ACTIVITY                    ( 3500001 );

constexpr MapActivityId
    ID_MAP_ACTIVITY_HARVEST			   (1),
	ID_MAP_ACTIVITY_RESOURCE           (2),
	ID_MAP_ACTIVITY_MONSTER            (3),
	ID_MAP_ACTIVITY_GOBLIN             (4),
	ID_MAP_ACTIVITY_KILL_SOLDIER       (5),
	ID_MAP_ACTIVITY_CASTLE_DAMAGE      (6),
	ID_MAP_ACTIVITY_END                (7);
}

}

#endif
