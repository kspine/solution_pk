#ifndef EMPERY_CENTER_EVENTS_RESOURCE_HPP_
#define EMPERY_CENTER_EVENTS_RESOURCE_HPP_

#include <poseidon/event_base.hpp>
#include "../id_types.hpp"

namespace EmperyCenter {

namespace Events {
	struct ResourceChanged : public Poseidon::EventBase<330201> {
		MapObjectUuid mapObjectUuid;
		AccountUuid ownerUuid;
		ResourceId resourceId;
		boost::uint64_t oldCount;
		boost::uint64_t newCount;

		ReasonId reason;
		boost::uint64_t param1;
		boost::uint64_t param2;
		boost::uint64_t param3;

		ResourceChanged(const MapObjectUuid &mapObjectUuid_, const AccountUuid &ownerUuid_,
			ResourceId resourceId_, boost::uint64_t oldCount_, boost::uint64_t newCount_,
			ReasonId reason_, boost::uint64_t param1_, boost::uint64_t param2_, boost::uint64_t param3_)
			: mapObjectUuid(mapObjectUuid_), ownerUuid(ownerUuid_)
			, resourceId(resourceId_), oldCount(oldCount_), newCount(newCount_)
			, reason(reason_), param1(param1_), param2(param2_), param3(param3_)
		{
		}
	};
}

}

#endif
