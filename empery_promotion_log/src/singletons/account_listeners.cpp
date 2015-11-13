#include "../precompiled.hpp"
#include <poseidon/singletons/event_dispatcher.hpp>
#include "../mysql/account_created.hpp"
#include "../mysql/account_logged_in.hpp"
#include "../../../empery_promotion/src/events/account.hpp"

namespace EmperyPromotionLog {

namespace {
	using namespace EmperyPromotion;

	MODULE_RAII_PRIORITY(handles, 5000){
		auto listener = Poseidon::EventDispatcher::register_listener<Events::AccountCreated>(
			[](const boost::shared_ptr<Events::AccountCreated> &event){
				const auto obj = boost::make_shared<MySql::PromotionLog_AccountCreated>(Poseidon::get_local_time(),
					event->account_id.get(), event->remote_ip);
				obj->async_save(true);
			});
		LOG_EMPERY_PROMOTION_LOG_DEBUG("Created AccountCreated listener");
		handles.push(std::move(listener));

		listener = Poseidon::EventDispatcher::register_listener<Events::AccountLoggedIn>(
			[](const boost::shared_ptr<Events::AccountLoggedIn> &event){
				const auto obj = boost::make_shared<MySql::PromotionLog_AccountLoggedIn>(Poseidon::get_local_time(),
					event->account_id.get(), event->remote_ip);
				obj->async_save(true);
			});
		LOG_EMPERY_PROMOTION_LOG_DEBUG("Created AccountLoggedIn listener");
		handles.push(std::move(listener));
	}
}

}
