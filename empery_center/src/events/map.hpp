#ifndef EMPERY_CENTER_EVENTS_MAP_HPP_
#define EMPERY_CENTER_EVENTS_MAP_HPP_

#include <poseidon/event_base.hpp>
#include "../id_types.hpp"
#include "../rectangle.hpp"

namespace EmperyCenter {

namespace Events {
	struct MapEventsOverflowed : public Poseidon::EventBase<340501> {
		Rectangle block_scope;
		std::uint64_t active_castles;
		std::uint64_t events_to_refresh;
		std::uint64_t events_retained;
		std::uint64_t events_created;

		MapEventsOverflowed(Rectangle block_scope_, std::uint64_t active_castles_,
			std::uint64_t events_to_refresh_, std::uint64_t events_retained_, std::uint64_t events_created_)
			: block_scope(block_scope_), active_castles(active_castles_)
			, events_to_refresh(events_to_refresh_), events_retained(events_retained_), events_created(events_created_)
		{
		}
	};
}

}

#endif
