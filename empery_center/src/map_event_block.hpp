#ifndef EMPERY_CENTER_MAP_EVENT_BLOCK_HPP_
#define EMPERY_CENTER_MAP_EVENT_BLOCK_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <boost/container/flat_map.hpp>
#include <vector>
#include "id_types.hpp"
#include "coord.hpp"
#include "rectangle.hpp"

namespace EmperyCenter {

namespace MySql {
	class Center_MapEventBlock;
	class Center_MapEvent;
}

class PlayerSession;

class MapEventBlock : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
public:
	enum : unsigned {
		BLOCK_WIDTH  = 30,
		BLOCK_HEIGHT = 32,
	};

	enum : unsigned {
		MAP_EVENT_COMMON = 1,
		MAP_EVENT_ACTIVITY_RESOUCE = 2,
		MAP_EVENT_ACTIVITY_GOBLIN = 3,
		MAP_EVENT_WORLD_ACTIVITY_MONSTER = 4,
		MAP_EVENT_COUNTRY_ACTIVIYT_RESOURCE = 5,
	};

	struct EventInfo {
		Coord coord;
		std::uint64_t created_time;
		std::uint64_t expiry_time;
		MapEventId map_event_id;
		Poseidon::Uuid meta_uuid;
	};

private:
	const boost::shared_ptr<MySql::Center_MapEventBlock> m_obj;

	boost::container::flat_map<Coord,
		boost::shared_ptr<MySql::Center_MapEvent>> m_events;

public:
	explicit MapEventBlock(Coord block_coord);
	MapEventBlock(boost::shared_ptr<MySql::Center_MapEventBlock> obj,
		const std::vector<boost::shared_ptr<MySql::Center_MapEvent>> &events);
	~MapEventBlock();

public:
	virtual void pump_status();

	void refresh_events(bool first_time,unsigned event_type = MAP_EVENT_COMMON);
	void remove_expired_events(boost::uint64_t utc_now,unsigned event_type = MAP_EVENT_COMMON,bool force = false);
	bool refresh_boss(MapObjectUuid boss_uuid,boost::uint64_t utc_now);

	Coord get_block_coord() const;
	Rectangle get_block_scope() const {
		return Rectangle(get_block_coord(), BLOCK_WIDTH, BLOCK_HEIGHT);
	}
	MapEventCircleId get_map_event_cicle_id() const;
	std::uint64_t get_next_refresh_time() const;

	EventInfo get(Coord coord) const;
	void get_all(std::vector<EventInfo> &ret) const;

	void insert(EventInfo info);
	void update(EventInfo info, bool throws_if_not_exists = true);
	bool remove(Coord coord) noexcept;
};

}

#endif
