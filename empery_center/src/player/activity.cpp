#include "../precompiled.hpp"
#include "common.hpp"
#include "../mmain.hpp"
#include "../msg/cs_activity.hpp"
#include "../msg/sc_activity.hpp"
#include "../singletons/activity_map.hpp"
#include "../activity.hpp"

namespace EmperyCenter {

PLAYER_SERVLET(Msg::CS_MapActivityInfo, account, session, /* req */){
	const auto map_activity = ActivityMap::get_map_activity();
	map_activity->synchronize_with_player(session);
	return Response();
}

}
