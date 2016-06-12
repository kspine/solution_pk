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
		MapObjectWeaponId map_object_weapon_id;
		MapObjectChassisId map_object_chassis_id;

		std::uint64_t max_soldier_count;
		double speed;
		std::uint64_t hp_per_soldier;
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
		boost::container::flat_map<ResourceId, double> maintenance_cost;

		MapObjectTypeId previous_id;
		double production_time;
		BuildingId factory_id;

		boost::container::flat_map<ResourceId, std::uint64_t> healing_cost;
		double healing_time;
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

		static void get_by_collection_name(
			std::vector<boost::shared_ptr<const MapObjectTypeMonsterReward>> &ret, const std::string &collection_name);
		static boost::shared_ptr<const MapObjectTypeMonsterReward> random_by_collection_name(
			const std::string &collection_name);

	public:
		std::uint64_t unique_id;
		std::string collection_name;
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

	class MapObjectTypeAttributeBonus {
	public:
		enum ApplicabilityKeyType {
			AKT_ALL                 = 1,
			AKT_CHASSIS_ID          = 2,
			AKT_WEAPON_ID           = 3,
			AKT_MAP_OBJECT_TYPE_ID  = 4,
		};

	public:
		static boost::shared_ptr<const MapObjectTypeAttributeBonus> get(std::uint64_t unique_id);
		static boost::shared_ptr<const MapObjectTypeAttributeBonus> require(std::uint64_t unique_id);

		static void get_applicable(std::vector<boost::shared_ptr<const MapObjectTypeAttributeBonus>> &ret,
			ApplicabilityKeyType applicability_key_type, std::uint64_t applicability_key_value);

	public:
		std::uint64_t unique_id;
		std::pair<unsigned, std::uint64_t> applicability_key;
		AttributeId tech_attribute_id;
		AttributeId bonus_attribute_id;
	};
}

}

#endif
