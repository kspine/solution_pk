#ifndef EMPERY_CENTER_EVENTS_ACCOUNT_HPP_
#define EMPERY_CENTER_EVENTS_ACCOUNT_HPP_

#include <poseidon/event_base.hpp>
#include "../id_types.hpp"

namespace EmperyCenter {

namespace Events {
	struct AccountCreated : public Poseidon::EventBase<330101> {
		AccountUuid accountUuid;
		std::string remoteIp;

		AccountCreated(const AccountUuid &accountUuid_, std::string remoteIp_)
			: accountUuid(accountUuid_), remoteIp(std::move(remoteIp_))
		{
		}
	};

	struct AccountLoggedIn : public Poseidon::EventBase<330102> {
		AccountUuid accountUuid;
		std::string remoteIp;

		AccountLoggedIn(const AccountUuid &accountUuid_, std::string remoteIp_)
			: accountUuid(accountUuid_), remoteIp(std::move(remoteIp_))
		{
		}
	};

	struct AccountLoggedOut : public Poseidon::EventBase<330103> {
		AccountUuid accountUuid;
		boost::uint64_t onlineDuration;

		AccountLoggedOut(const AccountUuid &accountUuid_, boost::uint64_t onlineDuration_)
			: accountUuid(accountUuid_), onlineDuration(onlineDuration_)
		{
		}
	};

	struct AccountSetToken : public Poseidon::EventBase<330104> {
		PlatformId platformId;
		std::string loginName;
		std::string loginToken;
		boost::uint64_t expiryTime;

		AccountSetToken(PlatformId platformId_, std::string loginName_, std::string loginToken_, boost::uint64_t expiryTime_)
			: platformId(platformId_), loginName(std::move(loginName_)), loginToken(std::move(loginToken_)), expiryTime(expiryTime_)
		{
		}
	};
}

}

#endif
