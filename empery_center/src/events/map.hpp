#ifndef EMPERY_CENTER_EVENTS_MAP_HPP_
#define EMPERY_CENTER_EVENTS_MAP_HPP_

#include <poseidon/event_base.hpp>
#include "../id_types.hpp"
#include "../rectangle.hpp"

namespace EmperyCenter {

namespace Events {
	struct MapEventsOverflowed : public Poseidon::EventBase<340501> {
		Rectangle block_scope;
		std::uint64_t active_castle_count;
		MapEventId map_event_id;
		std::uint64_t events_to_refresh;
		std::uint64_t events_retained;
		std::uint64_t events_created;

		MapEventsOverflowed(Rectangle block_scope_, std::uint64_t active_castle_count_, MapEventId map_event_id_,
			std::uint64_t events_to_refresh_, std::uint64_t events_retained_, std::uint64_t events_created_)
			: block_scope(block_scope_), active_castle_count(active_castle_count_), map_event_id(map_event_id_)
			, events_to_refresh(events_to_refresh_), events_retained(events_retained_), events_created(events_created_)
		{
		}
	};
}

}

#endif
