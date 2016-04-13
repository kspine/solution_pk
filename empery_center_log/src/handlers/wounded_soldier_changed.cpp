#include "../precompiled.hpp"
#include <poseidon/singletons/event_dispatcher.hpp>
#include "../mysql/wounded_soldier_changed.hpp"
#include "../../../empery_center/src/events/castle.hpp"

namespace EmperyCenterLog {

using namespace EmperyCenter;

MODULE_RAII_PRIORITY(handles, 5000){
	auto listener = Poseidon::EventDispatcher::register_listener<Events::WoundedSoldierChanged>(
		[](const boost::shared_ptr<Events::WoundedSoldierChanged> &event){
			const auto obj = boost::make_shared<MySql::CenterLog_WoundedSoldierChanged>(Poseidon::get_utc_time(),
				event->map_object_uuid.get(), event->owner_uuid.get(), event->map_object_type_id.get(), event->old_amount, event->new_amount,
				static_cast<std::int64_t>(event->new_amount - event->old_amount),
				event->reason.get(), event->param1, event->param2, event->param3);
			obj->async_save(false, true);
		});
	LOG_EMPERY_CENTER_LOG_DEBUG("Created WoundedSoldierChanged listener");
	handles.push(std::move(listener));
}

}
