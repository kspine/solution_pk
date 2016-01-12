#include "../precompiled.hpp"
#include <poseidon/singletons/event_dispatcher.hpp>
#include "../mysql/auction_item_changed.hpp"
#include "../../../empery_center/src/events/auction.hpp"

namespace EmperyCenterLog {

using namespace EmperyCenter;

MODULE_RAII_PRIORITY(handles, 5000){
	auto listener = Poseidon::EventDispatcher::register_listener<Events::AuctionItemChanged>(
		[](const boost::shared_ptr<Events::AuctionItemChanged> &event){
			const auto obj = boost::make_shared<MySql::CenterLog_AuctionItemChanged>(Poseidon::get_utc_time(),
				event->account_uuid.get(), event->item_id.get(), event->old_count, event->new_count,
				static_cast<std::int64_t>(event->new_count - event->old_count),
				event->reason.get(), event->param1, event->param2, event->param3);
			obj->async_save(false);
		});
	LOG_EMPERY_CENTER_LOG_DEBUG("Created AuctionItemChanged listener");
	handles.push(std::move(listener));
}

}
