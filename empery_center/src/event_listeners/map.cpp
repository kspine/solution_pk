#include "../precompiled.hpp"
#include "common.hpp"
#include "../events/map.hpp"
#include "../msg/sl_map.hpp"
#include "../singletons/log_client.hpp"

namespace EmperyCenter {

EVENT_LISTENER(Events::MapEventsOverflowed, event){
	const auto log = LogClient::require();

	Msg::SL_MapEventsOverflowed msg;
	msg.x                   = event.block_scope.left();
	msg.y                   = event.block_scope.bottom();
	msg.width               = event.block_scope.width();
	msg.height              = event.block_scope.height();
	msg.active_castle_count = event.active_castle_count;
	msg.map_event_id        = event.map_event_id.get();
	msg.events_to_refresh   = event.events_to_refresh;
	msg.events_retained     = event.events_retained;
	msg.events_created      = event.events_created;
	log->send(msg);
}

}
