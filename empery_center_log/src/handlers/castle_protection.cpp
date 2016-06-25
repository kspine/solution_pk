#include "../precompiled.hpp"
#include "common.hpp"
#include <poseidon/singletons/event_dispatcher.hpp>
#include "../mysql/castle_protection.hpp"
#include "../../../empery_center/src/events/castle.hpp"

namespace EmperyCenterLog {

MODULE_RAII_PRIORITY(handles, 5000){
	auto listener = Poseidon::EventDispatcher::register_listener<Events::CastleProtection>(
		[](const boost::shared_ptr<Events::CastleProtection> &event){
			const auto obj = boost::make_shared<MySql::CenterLog_CastleProtection>(Poseidon::get_utc_time(),
				event->map_object_uuid.get(), event->owner_uuid.get(),
				event->delta_preparation_duration, event->delta_protection_duration, event->protection_end);
			obj->async_save(false, true);
		});
	LOG_EMPERY_CENTER_LOG_DEBUG("Created CastleProtection listener");
	handles.push(std::move(listener));
}

}
