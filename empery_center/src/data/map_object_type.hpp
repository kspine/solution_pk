#ifndef EMPERY_CENTER_DATA_MAP_OBJECT_HPP_
#define EMPERY_CENTER_DATA_MAP_OBJECT_HPP_

#include "common.hpp"
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>

namespace EmperyCenter {

namespace Data {
	class MapObjectTypeAbstract {
	public:
		static boost::shared_ptr<const MapObjectTypeAbstract> get(MapObjectTypeId map_object_type_id);
		static boost::shared_ptr<const MapObjectTypeAbstract> require(MapObjectTypeId map_object_type_id);

	public:
		MapObjectTypeId map_object_type_id;
		MapObjectCategoryId map_object_category_id;

		std::uint64_t max_soldier_count;
		double speed;
	};

	class MapObjectTypeBattalion : public MapObjectTypeAbstract {
	public:
		static boost::shared_ptr<const MapObjectTypeBattalion> get(MapObjectTypeId map_object_type_id);
		static boost::shared_ptr<const MapObjectTypeBattalion> require(MapObjectTypeId map_object_type_id);

	public:
		double harvest_speed;
		double resource_carriable;

		boost::container::flat_map<BuildingId, unsigned> prerequisite;
		boost::container::flat_map<ResourceId, std::uint64_t> enability_cost;

		boost::container::flat_map<ResourceId, std::uint64_t> production_cost;
		boost::container::flat_map<ResourceId, std::uint64_t> maintenance_cost;

		MapObjectTypeId previous_id;
		double production_time;
		BuildingId factory_id;
	};

	class MapObjectTypeMonster : public MapObjectTypeAbstract {
	public:
		static boost::shared_ptr<const MapObjectTypeMonster> get(MapObjectTypeId map_object_type_id);
		static boost::shared_ptr<const MapObjectTypeMonster> require(MapObjectTypeId map_object_type_id);

	public:
		std::uint64_t return_range;

		boost::container::flat_map<std::string, std::uint64_t> monster_rewards;
	};

	class MapObjectTypeMonsterReward {
	public:
		static boost::shared_ptr<const MapObjectTypeMonsterReward> get(std::uint64_t unique_id);
		static boost::shared_ptr<const MapObjectTypeMonsterReward> require(std::uint64_t unique_id);

		static void get_by_name(std::vector<boost::shared_ptr<const MapObjectTypeMonsterReward>> &ret,
			const std::string &name);

	public:
		std::uint64_t unique_id;
		std::string name;
		double weight;
		boost::container::flat_map<ItemId, std::uint64_t> reward_items;
	};

	class MapObjectTypeMonsterRewardExtra {
	public:
		static boost::shared_ptr<const MapObjectTypeMonsterRewardExtra> get(std::uint64_t unique_id);
		static boost::shared_ptr<const MapObjectTypeMonsterRewardExtra> require(std::uint64_t unique_id);

		static void get_available(std::vector<boost::shared_ptr<const MapObjectTypeMonsterRewardExtra>> &ret,
			std::uint64_t utc_now, MapObjectTypeId monster_type_id);

	public:
		std::uint64_t unique_id;
		boost::container::flat_map<std::string, std::uint64_t> monster_rewards;
		boost::container::flat_set<MapObjectTypeId> monster_type_ids;
		std::uint64_t available_since;
		std::uint64_t available_until;
		std::uint64_t available_period;
		std::uint64_t available_duration;
	};
}

}

#endif
