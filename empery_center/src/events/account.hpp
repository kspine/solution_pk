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

	struct AccountPrecommitPaymentTransaction : public Poseidon::EventBase<340104> {
		AccountUuid account_uuid;
		ItemId item_id;
		std::uint64_t item_count;

		AccountPrecommitPaymentTransaction(AccountUuid account_uuid_, ItemId item_id_, std::uint64_t item_count_)
			: account_uuid(account_uuid_), item_id(item_id_), item_count(item_count_)
		{
		}
	};
}

}

#endif
