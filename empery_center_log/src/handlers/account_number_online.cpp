#include "../precompiled.hpp"
#include <poseidon/singletons/event_dispatcher.hpp>
#include "../mysql/account_number_online.hpp"
#include "../../../empery_center/src/events/account.hpp"

namespace EmperyCenterLog {

using namespace EmperyCenter;

MODULE_RAII_PRIORITY(handles, 5000){
	auto listener = Poseidon::EventDispatcher::register_listener<Events::AccountNumberOnline>(
		[](const boost::shared_ptr<Events::AccountNumberOnline> &event){
			const auto obj = boost::make_shared<MySql::CenterLog_AccountNumberOnline>(Poseidon::get_utc_time(),
				event->interval, event->account_count);
			obj->async_save(false);
		});
	LOG_EMPERY_CENTER_LOG_DEBUG("Created AccountNumberOnline listener");
	handles.push(std::move(listener));
}

}
