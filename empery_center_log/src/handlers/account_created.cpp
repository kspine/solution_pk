#include "../precompiled.hpp"
#include <poseidon/singletons/event_dispatcher.hpp>
#include "../mysql/account_created.hpp"
#include "../../../empery_center/src/events/account.hpp"

namespace EmperyCenterLog {

using namespace EmperyCenter;

MODULE_RAII_PRIORITY(handles, 5000){
	auto listener = Poseidon::EventDispatcher::register_listener<Events::AccountCreated>(
		[](const boost::shared_ptr<Events::AccountCreated> &event){
			const auto obj = boost::make_shared<MySql::CenterLog_AccountCreated>(Poseidon::get_utc_time(),
				event->account_uuid.get(), event->remote_ip);
			obj->async_save(false);
		});
	LOG_EMPERY_CENTER_LOG_DEBUG("Created AccountCreated listener");
	handles.push(std::move(listener));
}

}
