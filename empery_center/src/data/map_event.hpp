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

		static void get_all(std::vector<boost::shared_ptr<const MapEventBlock>> &ret);

	public:
		std::pair<unsigned, unsigned> num_coord;
		MapEventCircleId map_event_circle_id;
		std::uint64_t refresh_interval;
		unsigned priority;
	};

	class MapEventGeneration {
	public:
		static boost::shared_ptr<const MapEventGeneration> get(std::uint64_t unique_id);
		static boost::shared_ptr<const MapEventGeneration> require(std::uint64_t unique_id);
		static unsigned get_event_type(MapEventId event_id);

		static void get_by_map_event_circle_id_and_type(std::vector<boost::shared_ptr<const MapEventGeneration>> &ret,
			MapEventCircleId map_event_circle_id,unsigned map_event_type);

	public:
		std::uint64_t unique_id;
		MapEventCircleId map_event_circle_id;
		MapEventId map_event_id;
		unsigned   map_event_type;
		std::pair<unsigned,MapEventCircleId> type_circle;
		double event_count_multiplier;
		std::uint64_t expiry_duration;
		unsigned priority;
	};

	class MapEventAbstract {
	public:
		static boost::shared_ptr<const MapEventAbstract> get(MapEventId map_event_id);
		static boost::shared_ptr<const MapEventAbstract> require(MapEventId map_event_id);

	public:
		MapEventId map_event_id;
		boost::container::flat_set<TerrainId> restricted_terrains;
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
