#include "../precompiled.hpp"
#include <poseidon/singletons/event_dispatcher.hpp>
#include "../mysql/item_changed.hpp"
#include "../../../empery_promotion/src/events/item.hpp"

namespace EmperyPromotionLog {

namespace {
	using namespace EmperyPromotion;

	MODULE_RAII_PRIORITY(handles, 5000){
		auto listener = Poseidon::EventDispatcher::register_listener<Events::ItemChanged>(
			[](const boost::shared_ptr<Events::ItemChanged> &event){
				const auto obj = boost::make_shared<MySql::PromotionLog_ItemChanged>(Poseidon::get_local_time(),
					event->account_id.get(), event->item_id.get(), event->old_count, event->new_count,
					event->reason, event->param1, event->param2, event->param3);
				obj->async_save(true);
			});
		LOG_EMPERY_PROMOTION_LOG_DEBUG("Created ItemChanged listener");
		handles.push(std::move(listener));
	}
}

}
