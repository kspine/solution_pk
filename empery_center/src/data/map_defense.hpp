#ifndef EMPERY_CENTER_DATA_MAP_DEFENSE_HPP_
#define EMPERY_CENTER_DATA_MAP_DEFENSE_HPP_

#include "common.hpp"
#include <vector>
#include <boost/container/flat_map.hpp>

namespace EmperyCenter {

namespace Data {
	class MapDefenseBuildingAbstract {
	public:
		static boost::shared_ptr<const MapDefenseBuildingAbstract> get(MapObjectTypeId map_object_type_id, unsigned building_level);
		static boost::shared_ptr<const MapDefenseBuildingAbstract> require(MapObjectTypeId map_object_type_id, unsigned building_level);

	public:
		unsigned building_level;
		double upgrade_duration;
		boost::container::flat_map<ResourceId, std::uint64_t> upgrade_cost;
		boost::container::flat_map<BuildingId, unsigned> prerequisite;
		double destruct_duration;
		DefenseCombatId defense_combat_id;
		boost::container::flat_map<ResourceId, std::uint64_t> debris;
	};

	class MapDefenseBuildingCastle : public MapDefenseBuildingAbstract {
	public:
		static boost::shared_ptr<const MapDefenseBuildingCastle> get(unsigned building_level);
		static boost::shared_ptr<const MapDefenseBuildingCastle> require(unsigned building_level);

	public:
		//
	};

	class MapDefenseBuildingDefenseTower : public MapDefenseBuildingAbstract {
	public:
		static boost::shared_ptr<const MapDefenseBuildingDefenseTower> get(unsigned building_level);
		static boost::shared_ptr<const MapDefenseBuildingDefenseTower> require(unsigned building_level);

	public:
		//
	};

	class MapDefenseBuildingBattleBunker : public MapDefenseBuildingAbstract {
	public:
		static boost::shared_ptr<const MapDefenseBuildingBattleBunker> get(unsigned building_level);
		static boost::shared_ptr<const MapDefenseBuildingBattleBunker> require(unsigned building_level);

	public:
		//
	};

	class MapDefenseCombat {
	public:
		static boost::shared_ptr<const MapDefenseCombat> get(DefenseCombatId defense_combat_id);
		static boost::shared_ptr<const MapDefenseCombat> require(DefenseCombatId defense_combat_id);

	public:
		DefenseCombatId defense_combat_id;
		MapObjectWeaponId map_object_weapon_id;
		std::uint64_t soldiers_max;
		double self_healing_rate;
		std::uint64_t hp_per_soldier;
	};
}

}

#endif
