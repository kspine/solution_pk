#include "../precompiled.hpp"
#include "common.hpp"
#include <poseidon/singletons/event_dispatcher.hpp>
#include "../mysql/map_events_overflowed.hpp"
#include "../../../empery_center/src/events/map.hpp"

namespace EmperyCenterLog {

MODULE_RAII_PRIORITY(handles, 5000){
	auto listener = Poseidon::EventDispatcher::register_listener<Events::MapEventsOverflowed>(
		[](const boost::shared_ptr<Events::MapEventsOverflowed> &event){
			const auto obj = boost::make_shared<MySql::CenterLog_MapEventsOverflowed>(Poseidon::get_utc_time(),
				event->block_scope.left(), event->block_scope.bottom(), event->block_scope.width(), event->block_scope.height(),
				event->active_castle_count, event->map_event_id.get(),
				event->events_to_refresh, event->events_retained, event->events_created);
			obj->async_save(false, true);
		});
	LOG_EMPERY_CENTER_LOG_DEBUG("Created MapEventsOverflowed listener");
	handles.push(std::move(listener));
}

}
