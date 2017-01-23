#include "../precompiled.hpp"
#include <poseidon/singletons/event_dispatcher.hpp>
#include "../mysql/synuser.hpp"
#include "../../../empery_promotion/src/events/synuser.hpp"

namespace EmperyPromotionLog {

namespace {
	using namespace EmperyPromotion;

	MODULE_RAII_PRIORITY(handles, 5000){
		auto listener = Poseidon::EventDispatcher::register_listener<Events::SynUser>(
			[](const boost::shared_ptr<Events::SynUser> &event){
				const auto obj = boost::make_shared<MySql::PromotionLog_SynUser>(Poseidon::get_utc_time(),
					event->remote_ip, event->url, event->params);
				obj->async_save(true);
			});
		LOG_EMPERY_PROMOTION_LOG_DEBUG("Created SynUser listener");
		handles.push(std::move(listener));
	}
}

}
