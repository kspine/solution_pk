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
		boost::shared_ptr<long> error_code;
		PlatformId platform_id;
		std::string login_name;
		std::string third_token;

		AccountSynchronizeWithThirdServer(boost::shared_ptr<long> error_code_,
			PlatformId platform_id_, std::string login_name_, std::string third_token_)
			: error_code(std::move(error_code_))
			, platform_id(platform_id_), login_name(std::move(login_name_)), third_token(std::move(third_token_))
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

	struct AccountInvalidate : public Poseidon::EventBase<340106> {
		AccountUuid account_uuid;

		explicit AccountInvalidate(AccountUuid account_uuid_)
			: account_uuid(account_uuid_)
		{
		}
	};
}

}

#endif
