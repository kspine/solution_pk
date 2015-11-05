#include "../precompiled.hpp"
#include <poseidon/singletons/event_dispatcher.hpp>
#include "../mysql/item_changed.hpp"
#include "../../../empery_center/src/events/item.hpp"

namespace EmperyCenterLog {

namespace {
	using namespace EmperyCenter;

	MODULE_RAII_PRIORITY(handles, 5000){
		auto listener = Poseidon::EventDispatcher::registerListener<Events::ItemChanged>(
			[](const boost::shared_ptr<Events::ItemChanged> &event){
				const auto obj = boost::make_shared<MySql::CenterLog_ItemChanged>(Poseidon::getLocalTime(),
					event->accountUuid.get(), event->itemId.get(), event->oldCount, event->newCount,
					event->reason.get(), event->param1, event->param2, event->param3);
				obj->asyncSave(true);
			});
		LOG_EMPERY_CENTER_LOG_DEBUG("Created ItemChanged listener");
		handles.push(std::move(listener));
	}
}

}
