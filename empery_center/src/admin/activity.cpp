#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/kill.hpp"
#include "../singletons/player_session_map.hpp"
#include "../singletons/activity_map.hpp"
#include "../player_session.hpp"

namespace EmperyCenter {

ADMIN_SERVLET("activity/change", root, session, params){
	ActivityMap::reload();
	return Response();
}

}

