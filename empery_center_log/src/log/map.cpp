#include "../precompiled.hpp"
#include "common.hpp"
#include "../../../empery_center/src/msg/sl_map.hpp"
#include "../mysql/map_events_overflowed.hpp"

namespace EmperyCenterLog {

LOG_SERVLET(Msg::SL_MapEventsOverflowed, log, req){
	auto obj = boost::make_shared<MySql::CenterLog_MapEventsOverflowed>(
		Poseidon::get_utc_time(),
		req.block_x, req.block_y, req.width, req.height, req.active_castle_count, req.map_event_id,
		req.events_to_refresh, req.events_retained, req.events_created);
	obj->async_save(false, true);
}

}
