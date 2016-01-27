#ifndef EMPERY_CENTER_DATA_CASTLE_HPP_
#define EMPERY_CENTER_DATA_CASTLE_HPP_

#include "common.hpp"
#include <boost/container/flat_set.hpp>
#include <boost/container/flat_map.hpp>

namespace EmperyCenter {

namespace Data {
	class CastleBuildingBase {
	public:
		static boost::shared_ptr<const CastleBuildingBase> get(BuildingBaseId building_base_id);
		static boost::shared_ptr<const CastleBuildingBase> require(BuildingBaseId building_base_id);

		// 返回所有 init_level > 0 的建筑。
		static void get_init(std::vector<boost::shared_ptr<const CastleBuildingBase>> &ret);

	public:
		BuildingBaseId building_base_id;
		boost::container::flat_set<BuildingId> buildings_allowed;
		unsigned init_level;
	};

	class CastleBuilding {
	public:
		enum Type {
			T_PRIMARY           =  1,
			T_ACADEMY           =  3,
			T_CIVILIAN          =  4,
			T_WAREHOUSE         =  6,
			T_CITADEL_WALL      =  7,
			T_DEFENSE_TOWER     =  8,
			T_STABLES           = 17,
			T_BARRACKS          = 18,
			T_ARCHER_BARRACKS   = 19,
			T_WEAPONRY          = 20,
		};

	public:
		static boost::shared_ptr<const CastleBuilding> get(BuildingId building_id);
		static boost::shared_ptr<const CastleBuilding> require(BuildingId building_id);

	public:
		BuildingId building_id;
		unsigned build_limit;
		Type type;
	};

	class CastleUpgradeAbstract {
	public:
		static boost::shared_ptr<const CastleUpgradeAbstract> get(CastleBuilding::Type type, unsigned building_level);
		static boost::shared_ptr<const CastleUpgradeAbstract> require(CastleBuilding::Type type, unsigned building_level);

	public:
		unsigned building_level;
		double upgrade_duration;
		boost::container::flat_map<ResourceId, std::uint64_t> upgrade_cost;
		boost::container::flat_map<BuildingId, unsigned> prerequisite;
		std::uint64_t prosperity_points;
		double destruct_duration;
	};

	class CastleUpgradePrimary : public CastleUpgradeAbstract {
	public:
		static boost::shared_ptr<const CastleUpgradePrimary> get(unsigned building_level);
		static boost::shared_ptr<const CastleUpgradePrimary> require(unsigned building_level);

	public:
		std::uint64_t max_map_cell_count;
		std::uint64_t max_map_cell_distance;
		std::uint64_t max_immigrant_group_count;
	};

	class CastleUpgradeStables : public CastleUpgradeAbstract {
	public:
		static boost::shared_ptr<const CastleUpgradeStables> get(unsigned building_level);
		static boost::shared_ptr<const CastleUpgradeStables> require(unsigned building_level);

	public:
		//
	};

	class CastleUpgradeBarracks : public CastleUpgradeAbstract {
	public:
		static boost::shared_ptr<const CastleUpgradeBarracks> get(unsigned building_level);
		static boost::shared_ptr<const CastleUpgradeBarracks> require(unsigned building_level);

	public:
		//
	};

	class CastleUpgradeArcherBarracks : public CastleUpgradeAbstract {
	public:
		static boost::shared_ptr<const CastleUpgradeArcherBarracks> get(unsigned building_level);
		static boost::shared_ptr<const CastleUpgradeArcherBarracks> require(unsigned building_level);

	public:
		//
	};

	class CastleUpgradeWeaponry : public CastleUpgradeAbstract {
	public:
		static boost::shared_ptr<const CastleUpgradeWeaponry> get(unsigned building_level);
		static boost::shared_ptr<const CastleUpgradeWeaponry> require(unsigned building_level);

	public:
		//
	};

	class CastleUpgradeAcademy : public CastleUpgradeAbstract {
	public:
		static boost::shared_ptr<const CastleUpgradeAcademy> get(unsigned building_level);
		static boost::shared_ptr<const CastleUpgradeAcademy> require(unsigned building_level);

	public:
		//
	};

	class CastleUpgradeCivilian : public CastleUpgradeAbstract {
	public:
		static boost::shared_ptr<const CastleUpgradeCivilian> get(unsigned building_level);
		static boost::shared_ptr<const CastleUpgradeCivilian> require(unsigned building_level);

	public:
		double population_production_rate;
		std::uint64_t population_capacity;
	};

	class CastleUpgradeWarehouse : public CastleUpgradeAbstract {
	public:
		static boost::shared_ptr<const CastleUpgradeWarehouse> get(unsigned building_level);
		static boost::shared_ptr<const CastleUpgradeWarehouse> require(unsigned building_level);

	public:
		boost::container::flat_map<ResourceId, std::uint64_t> max_resource_amounts;
	};

	class CastleUpgradeCitadelWall : public CastleUpgradeAbstract {
	public:
		static boost::shared_ptr<const CastleUpgradeCitadelWall> get(unsigned building_level);
		static boost::shared_ptr<const CastleUpgradeCitadelWall> require(unsigned building_level);

	public:
		std::uint64_t strength;
		std::uint64_t armor;
	};

	class CastleUpgradeDefenseTower : public CastleUpgradeAbstract {
	public:
		static boost::shared_ptr<const CastleUpgradeDefenseTower> get(unsigned building_level);
		static boost::shared_ptr<const CastleUpgradeDefenseTower> require(unsigned building_level);

	public:
		std::uint64_t firepower;
	};

	class CastleTech {
	public:
		static boost::shared_ptr<const CastleTech> get(TechId tech_id, unsigned tech_level);
		static boost::shared_ptr<const CastleTech> require(TechId tech_id, unsigned tech_level);

	public:
		std::pair<TechId, unsigned> tech_id_level;
		double upgrade_duration;
		boost::container::flat_map<ResourceId, std::uint64_t> upgrade_cost;
		boost::container::flat_map<BuildingId, unsigned> prerequisite;
		boost::container::flat_map<BuildingId, unsigned> display_prerequisite;
		boost::container::flat_map<AttributeId, double> attributes;
	};

	class CastleResource {
	public:
		static boost::shared_ptr<const CastleResource> get(ResourceId resource_id);
		static boost::shared_ptr<const CastleResource> require(ResourceId resource_id);

		static boost::shared_ptr<const CastleResource> get_by_locked_resource_id(ResourceId locked_resource_id);
		static boost::shared_ptr<const CastleResource> require_by_locked_resource_id(ResourceId locked_resource_id);

		static void get_all(std::vector<boost::shared_ptr<const CastleResource>> &ret);

	public:
		ResourceId resource_id;
		std::uint64_t init_amount;
		bool producible;
		ResourceId locked_resource_id;
		ItemId undeployed_item_id;
	};
}

}

#endif
