#ifndef EMPERY_PROMOTION_EVENTS_ADMIN_HPP_
#define EMPERY_PROMOTION_EVENTS_ADMIN_HPP_

#include <poseidon/event_base.hpp>
#include <poseidon/shared_nts.hpp>

namespace EmperyPromotion {

namespace Events {
	struct Admin : public Poseidon::EventBase<330401> {
		std::string remote_ip;
		std::string url;
		std::string params;

		Admin(std::string remote_ip_, std::string url_, std::string params_)
			: remote_ip(std::move(remote_ip_)), url(std::move(url_)), params(std::move(params_))
		{
		}
	};
}

}

#endif
