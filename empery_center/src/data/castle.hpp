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
			T_PRIMARY       = 1,
			T_BARRACKS      = 2,
			T_ACADEMY       = 3,
			T_CIVILIAN      = 4,
			// T_WATCHTOWER    = 5,
			T_WAREHOUSE     = 6,
			T_CITADEL_WALL  = 7,
			T_DEFENSE_TOWER = 8,
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
		boost::uint64_t upgrade_duration;
		boost::container::flat_map<ResourceId, boost::uint64_t> upgrade_cost;
		boost::container::flat_map<BuildingId, unsigned> prerequisite;
		boost::uint64_t prosperity_points;
		boost::uint64_t destruct_duration;
	};

	class CastleUpgradePrimary : public CastleUpgradeAbstract {
	public:
		static boost::shared_ptr<const CastleUpgradePrimary> get(unsigned building_level);
		static boost::shared_ptr<const CastleUpgradePrimary> require(unsigned building_level);

	public:
		boost::uint64_t max_map_cell_count;
		boost::uint64_t max_map_cell_distance;
		boost::uint64_t max_immigrant_group_count;
	};

	class CastleUpgradeBarracks : public CastleUpgradeAbstract {
	public:
		static boost::shared_ptr<const CastleUpgradeBarracks> get(unsigned building_level);
		static boost::shared_ptr<const CastleUpgradeBarracks> require(unsigned building_level);

	public:
		//
	};

	class CastleUpgradeAcademy : public CastleUpgradeAbstract {
	public:
		static boost::shared_ptr<const CastleUpgradeAcademy> get(unsigned building_level);
		static boost::shared_ptr<const CastleUpgradeAcademy> require(unsigned building_level);

	public:
		unsigned tech_level;
	};

	class CastleUpgradeCivilian : public CastleUpgradeAbstract {
	public:
		static boost::shared_ptr<const CastleUpgradeCivilian> get(unsigned building_level);
		static boost::shared_ptr<const CastleUpgradeCivilian> require(unsigned building_level);

	public:
		boost::uint64_t max_population;
	};

	class CastleUpgradeWarehouse : public CastleUpgradeAbstract {
	public:
		static boost::shared_ptr<const CastleUpgradeWarehouse> get(unsigned building_level);
		static boost::shared_ptr<const CastleUpgradeWarehouse> require(unsigned building_level);

	public:
		boost::container::flat_map<ResourceId, boost::uint64_t> max_resource_amounts;
	};

	class CastleUpgradeCitadelWall : public CastleUpgradeAbstract {
	public:
		static boost::shared_ptr<const CastleUpgradeCitadelWall> get(unsigned building_level);
		static boost::shared_ptr<const CastleUpgradeCitadelWall> require(unsigned building_level);

	public:
		boost::uint64_t strength;
		boost::uint64_t armor;
	};

	class CastleUpgradeDefenseTower : public CastleUpgradeAbstract {
	public:
		static boost::shared_ptr<const CastleUpgradeDefenseTower> get(unsigned building_level);
		static boost::shared_ptr<const CastleUpgradeDefenseTower> require(unsigned building_level);

	public:
		boost::uint64_t firepower;
	};

	class CastleTech {
	public:
		static boost::shared_ptr<const CastleTech> get(TechId tech_id, unsigned tech_level);
		static boost::shared_ptr<const CastleTech> require(TechId tech_id, unsigned tech_level);

	public:
		std::pair<TechId, unsigned> tech_id_level;
		boost::uint64_t upgrade_duration;
		boost::container::flat_map<ResourceId, boost::uint64_t> upgrade_cost;
		boost::container::flat_map<BuildingId, unsigned> prerequisite;
		boost::container::flat_map<BuildingId, unsigned> display_prerequisite;
		boost::container::flat_map<AttributeId, double> attributes;
	};
}

}

#endif
