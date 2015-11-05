#include "../precompiled.hpp"
#include <poseidon/singletons/event_dispatcher.hpp>
#include "../mysql/resource_changed.hpp"
#include "../../../empery_center/src/events/resource.hpp"

namespace EmperyCenterLog {

namespace {
	using namespace EmperyCenter;

	MODULE_RAII_PRIORITY(handles, 5000){
		auto listener = Poseidon::EventDispatcher::registerListener<Events::ResourceChanged>(
			[](const boost::shared_ptr<Events::ResourceChanged> &event){
				const auto obj = boost::make_shared<MySql::CenterLog_ResourceChanged>(Poseidon::getLocalTime(),
					event->mapObjectUuid.get(), event->ownerUuid.get(), event->resourceId.get(), event->oldCount, event->newCount,
					event->reason.get(), event->param1, event->param2, event->param3);
				obj->asyncSave(true);
			});
		LOG_EMPERY_CENTER_LOG_DEBUG("Created ResourceChanged listener");
		handles.push(std::move(listener));
	}
}

}
