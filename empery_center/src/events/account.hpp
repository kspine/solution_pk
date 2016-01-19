#ifndef EMPERY_CENTER_EVENTS_ACCOUNT_HPP_
#define EMPERY_CENTER_EVENTS_ACCOUNT_HPP_

#include <poseidon/event_base.hpp>
#include "../id_types.hpp"

namespace EmperyCenter {

namespace Events {
	struct AccountCreated : public Poseidon::EventBase<340101> {
		AccountUuid account_uuid;
		std::string remote_ip;

		AccountCreated(AccountUuid account_uuid_, std::string remote_ip_)
			: account_uuid(account_uuid_), remote_ip(std::move(remote_ip_))
		{
		}
	};

	struct AccountLoggedIn : public Poseidon::EventBase<340102> {
		AccountUuid account_uuid;
		std::string remote_ip;

		AccountLoggedIn(AccountUuid account_uuid_, std::string remote_ip_)
			: account_uuid(account_uuid_), remote_ip(std::move(remote_ip_))
		{
		}
	};

	struct AccountLoggedOut : public Poseidon::EventBase<340103> {
		AccountUuid account_uuid;
		std::uint64_t online_duration;

		AccountLoggedOut(AccountUuid account_uuid_, std::uint64_t online_duration_)
			: account_uuid(account_uuid_), online_duration(online_duration_)
		{
		}
	};

	struct AccountSynchronizeWithThirdServer : public Poseidon::EventBase<340104> {
		PlatformId platform_id;
		std::string login_name;

		AccountSynchronizeWithThirdServer(PlatformId platform_id_, std::string login_name_)
			: platform_id(platform_id_), login_name(std::move(login_name_))
		{
		}
	};

	struct AccountNumberOnline : public Poseidon::EventBase<340105> {
		std::uint64_t interval;
		std::uint64_t account_count;

		AccountNumberOnline(std::uint64_t interval_, std::uint64_t account_count_)
			: interval(interval_), account_count(account_count_)
		{
		}
	};
}

}

#endif
