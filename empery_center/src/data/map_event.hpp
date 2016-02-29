
#ifndef EMPERY_CENTER_DATA_MAP_EVENT_HPP_
#define EMPERY_CENTER_DATA_MAP_EVENT_HPP_

#include "common.hpp"
#include <vector>

namespace EmperyCenter {

namespace Data {
	class MapEventBlock {
	public:
		static boost::shared_ptr<const MapEventBlock> get(unsigned num_x, unsigned num_y);
		static boost::shared_ptr<const MapEventBlock> require(unsigned num_x, unsigned num_y);

	public:
		std::pair<unsigned, unsigned> num_coord;
		MapEventCircleId map_event_circle_id;
		std::uint64_t refresh_interval;
		unsigned priority;
	};

	class MapEventGeneration {
	public:
		static boost::shared_ptr<const MapEventGeneration> get(MapEventCircleId map_event_circle_id, MapEventId map_event_id);
		static boost::shared_ptr<const MapEventGeneration> require(MapEventCircleId map_event_circle_id, MapEventId map_event_id);

		static void get_by_map_event_circle_id(std::vector<boost::shared_ptr<const MapEventGeneration>> &ret,
			MapEventCircleId map_event_circle_id);

	public:
		std::pair<MapEventCircleId, MapEventId> event_circle_key;
		std::uint64_t event_count;
		std::uint64_t expiry_time;
		unsigned priority;
	};

	class MapEventAbstract {
	public:
		static boost::shared_ptr<const MapEventAbstract> get(MapEventId map_event_id);
		static boost::shared_ptr<const MapEventAbstract> require(MapEventId map_event_id);

	public:
		MapEventId map_event_id;
		TerrainId restricted_terrain_id;
	};

	class MapEventResource : public MapEventAbstract {
	public:
		static boost::shared_ptr<const MapEventResource> get(MapEventId map_event_id);
		static boost::shared_ptr<const MapEventResource> require(MapEventId map_event_id);

	public:
		ResourceId resource_id;
		std::uint64_t resource_amount;
	};

	class MapEventMonster : public MapEventAbstract {
	public:
		static boost::shared_ptr<const MapEventMonster> get(MapEventId map_event_id);
		static boost::shared_ptr<const MapEventMonster> require(MapEventId map_event_id);

	public:
		MapObjectTypeId monster_type_id;
	};
}

}

#endif
