#include "../precompiled.hpp"
#include <poseidon/singletons/event_dispatcher.hpp>
#include "../mysql/resource_changed.hpp"
#include "../../../empery_center/src/events/resource.hpp"

namespace EmperyCenterLog {

using namespace EmperyCenter;

MODULE_RAII_PRIORITY(handles, 5000){
	auto listener = Poseidon::EventDispatcher::register_listener<Events::ResourceChanged>(
		[](const boost::shared_ptr<Events::ResourceChanged> &event){
			const auto obj = boost::make_shared<MySql::CenterLog_ResourceChanged>(Poseidon::get_local_time(),
				event->map_object_uuid.get(), event->owner_uuid.get(), event->resource_id.get(), event->old_count, event->new_count,
				static_cast<boost::int64_t>(event->new_count - event->old_count),
				event->reason.get(), event->param1, event->param2, event->param3);
			obj->async_save(true);
		});
	LOG_EMPERY_CENTER_LOG_DEBUG("Created ResourceChanged listener");
	handles.push(std::move(listener));
}

}
