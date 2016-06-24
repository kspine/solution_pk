#ifndef EMPERY_CENTER_DATA_MAP_CELL_HPP_
#define EMPERY_CENTER_DATA_MAP_CELL_HPP_

#include "common.hpp"
#include <vector>
#include <boost/container/flat_set.hpp>

namespace EmperyCenter {

namespace Data {
	class MapCellBasic {
	public:
		static boost::shared_ptr<const MapCellBasic> get(unsigned map_x, unsigned map_y);
		static boost::shared_ptr<const MapCellBasic> require(unsigned map_x, unsigned map_y);

	public:
		std::pair<unsigned, unsigned> map_coord;
		TerrainId terrain_id;
	};

	class MapCellTicket {
	public:
		static boost::shared_ptr<const MapCellTicket> get(ItemId ticket_item_id);
		static boost::shared_ptr<const MapCellTicket> require(ItemId ticket_item_id);

	public:
		ItemId ticket_item_id;
		double production_rate_modifier;
		double capacity_modifier;
		std::uint64_t soldiers_max;
		double self_healing_rate;
		bool protectable;
	};

	class MapTerrain {
	public:
		static boost::shared_ptr<const MapTerrain> get(TerrainId terrain_id);
		static boost::shared_ptr<const MapTerrain> require(TerrainId terrain_id);

	public:
		TerrainId terrain_id;
		ResourceId best_resource_id;
		double best_production_rate; // 每分钟产出资源。
		std::uint64_t best_capacity;
		bool buildable;
		bool passable;
		std::uint64_t protection_cost;
	};

	class MapStartPoint {
	public:
		static boost::shared_ptr<const MapStartPoint> get(StartPointId start_point_id);
		static boost::shared_ptr<const MapStartPoint> require(StartPointId start_point_id);

		static void get_all(std::vector<boost::shared_ptr<const MapStartPoint>> &ret);

	public:
		StartPointId start_point_id;
		std::pair<unsigned, unsigned> map_coord;
	};

	class MapCrate {
	public:
		static boost::shared_ptr<const MapCrate> get(CrateId crate_id);
		static boost::shared_ptr<const MapCrate> require(CrateId crate_id);

		static boost::shared_ptr<const MapCrate> get_by_resource_amount(ResourceId resource_id, std::uint64_t amount);

	public:
		CrateId crate_id;
		std::pair<ResourceId, std::uint64_t> resource_amount_key;
	};

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
