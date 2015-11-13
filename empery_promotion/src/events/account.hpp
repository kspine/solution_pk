#ifndef EMPERY_PROMOTION_EVENTS_ACCOUNT_HPP_
#define EMPERY_PROMOTION_EVENTS_ACCOUNT_HPP_

#include <poseidon/event_base.hpp>
#include <poseidon/shared_nts.hpp>
#include "../id_types.hpp"

namespace EmperyPromotion {

namespace Events {
	struct AccountCreated : public Poseidon::EventBase<330101> {
		AccountId account_id;
		std::string remote_ip;

		AccountCreated(AccountId account_id_, std::string remote_ip_)
			: account_id(account_id_), remote_ip(std::move(remote_ip_))
		{
		}
	};
	struct AccountLoggedIn : public Poseidon::EventBase<330102> {
		AccountId account_id;
		std::string remote_ip;

		AccountLoggedIn(AccountId account_id_, std::string remote_ip_)
			: account_id(account_id_), remote_ip(std::move(remote_ip_))
		{
		}
	};
}

}

#endif
