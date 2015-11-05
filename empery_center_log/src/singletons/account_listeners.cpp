#include "../precompiled.hpp"
#include <poseidon/singletons/event_dispatcher.hpp>
#include "../mysql/account_created.hpp"
#include "../mysql/account_logged_in.hpp"
#include "../../../empery_center/src/events/account.hpp"

namespace EmperyCenterLog {

namespace {
	using namespace EmperyCenter;

	MODULE_RAII_PRIORITY(handles, 5000){
		auto listener = Poseidon::EventDispatcher::registerListener<Events::AccountCreated>(
			[](const boost::shared_ptr<Events::AccountCreated> &event){
				const auto obj = boost::make_shared<MySql::CenterLog_AccountCreated>(Poseidon::getLocalTime(),
					event->accountUuid.get(), event->remoteIp);
				obj->asyncSave(true);
			});
		LOG_EMPERY_CENTER_LOG_DEBUG("Created AccountCreated listener");
		handles.push(std::move(listener));

		listener = Poseidon::EventDispatcher::registerListener<Events::AccountLoggedIn>(
			[](const boost::shared_ptr<Events::AccountLoggedIn> &event){
				const auto obj = boost::make_shared<MySql::CenterLog_AccountLoggedIn>(Poseidon::getLocalTime(),
					event->accountUuid.get(), event->remoteIp);
				obj->asyncSave(true);
			});
		LOG_EMPERY_CENTER_LOG_DEBUG("Created AccountLoggedIn listener");
		handles.push(std::move(listener));
	}
}

}
