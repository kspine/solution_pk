#ifndef EMPERY_CENTER_EVENTS_RESOURCE_HPP_
#define EMPERY_CENTER_EVENTS_RESOURCE_HPP_

#include <poseidon/event_base.hpp>
#include "../id_types.hpp"

namespace EmperyCenter {

namespace Events {
	struct ResourceChanged : public Poseidon::EventBase<340301> {
		MapObjectUuid map_object_uuid;
		AccountUuid owner_uuid;
		ResourceId resource_id;
		boost::uint64_t old_amount;
		boost::uint64_t new_amount;

		ReasonId reason;
		boost::int64_t param1;
		boost::int64_t param2;
		boost::int64_t param3;

		ResourceChanged(MapObjectUuid map_object_uuid_, AccountUuid owner_uuid_,
			ResourceId resource_id_, boost::uint64_t old_amount_, boost::uint64_t new_amount_,
			ReasonId reason_, boost::int64_t param1_, boost::int64_t param2_, boost::int64_t param3_)
			: map_object_uuid(map_object_uuid_), owner_uuid(owner_uuid_)
			, resource_id(resource_id_), old_amount(old_amount_), new_amount(new_amount_)
			, reason(reason_), param1(param1_), param2(param2_), param3(param3_)
		{
		}
	};
}

}

#endif
