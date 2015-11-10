#ifndef EMPERY_CENTER_EVENTS_ITEM_HPP_
#define EMPERY_CENTER_EVENTS_ITEM_HPP_

#include <poseidon/event_base.hpp>
#include "../id_types.hpp"

namespace EmperyCenter {

namespace Events {
	struct ItemChanged : public Poseidon::EventBase<340201> {
		AccountUuid accountUuid;
		ItemId itemId;
		boost::uint64_t oldCount;
		boost::uint64_t newCount;

		ReasonId reason;
		boost::uint64_t param1;
		boost::uint64_t param2;
		boost::uint64_t param3;

		ItemChanged(const AccountUuid &accountUuid_,
			ItemId itemId_, boost::uint64_t oldCount_, boost::uint64_t newCount_,
			ReasonId reason_, boost::uint64_t param1_, boost::uint64_t param2_, boost::uint64_t param3_)
			: accountUuid(accountUuid_)
			, itemId(itemId_), oldCount(oldCount_), newCount(newCount_)
			, reason(reason_), param1(param1_), param2(param2_), param3(param3_)
		{
		}
	};
}

}

#endif
