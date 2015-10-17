#ifndef EMPERY_PROMOTION_EVENTS_ACCOUNT_HPP_
#define EMPERY_PROMOTION_EVENTS_ACCOUNT_HPP_

#include <poseidon/event_base.hpp>
#include <poseidon/shared_nts.hpp>
#include "../id_types.hpp"

namespace EmperyPromotion {

namespace Events {
	struct AccountCreated : public Poseidon::EventBase<330101> {
		AccountId accountId;
		std::string remoteIp;

		AccountCreated(AccountId accountId_, std::string remoteIp_)
			: accountId(accountId_), remoteIp(std::move(remoteIp_))
		{
		}
	};
	struct AccountLoggedIn : public Poseidon::EventBase<330102> {
		AccountId accountId;
		std::string remoteIp;

		AccountLoggedIn(AccountId accountId_, std::string remoteIp_)
			: accountId(accountId_), remoteIp(std::move(remoteIp_))
		{
		}
	};
}

}

#endif
