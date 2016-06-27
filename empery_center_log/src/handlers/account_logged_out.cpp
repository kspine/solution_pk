#include "../precompiled.hpp"
#include "common.hpp"
#include <poseidon/singletons/event_dispatcher.hpp>
#include "../mysql/account_logged_out.hpp"
#include "../../../empery_center/src/events/account.hpp"

namespace EmperyCenterLog {

MODULE_RAII_PRIORITY(handles, 5000){
	auto listener = Poseidon::EventDispatcher::register_listener<Events::AccountLoggedOut>(
		[](const boost::shared_ptr<Events::AccountLoggedOut> &event){
			const auto obj = boost::make_shared<MySql::CenterLog_AccountLoggedOut>(Poseidon::get_utc_time(),
				event->account_uuid.get(), event->online_duration);
			obj->async_save(false, true);
		});
	LOG_EMPERY_CENTER_LOG_DEBUG("Created AccountLoggedOut listener");
	handles.push(std::move(listener));
}

}
