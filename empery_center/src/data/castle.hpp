#ifndef EMPERY_CENTER_DATA_SNG_LEVEL_HPP_
#define EMPERY_CENTER_DATA_SNG_LEVEL_HPP_

#include "common.hpp"
#include <boost/container/flat_set.hpp>
#include <boost/container/flat_map.hpp>

namespace EmperyCenter {

namespace Data {
	class CastleBuildingBase {
	public:
		static boost::shared_ptr<const CastleBuildingBase> get(BuildingBaseId buildingBaseId);
		static boost::shared_ptr<const CastleBuildingBase> require(BuildingBaseId buildingBaseId);

		// 返回所有 initLevel > 0 的建筑。
		static void getInit(std::vector<boost::shared_ptr<const CastleBuildingBase>> &ret);

	public:
		BuildingBaseId buildingBaseId;
		boost::container::flat_set<BuildingId> buildingsAllowed;
		unsigned initLevel;
	};

	class CastleBuilding {
	public:
		enum Type {
			T_PRIMARY       = 1,
			T_BARRACKS      = 2,
			T_ACADEMY       = 3,
			T_CIVILIAN      = 4,
//			T_WATCHTOWER    = 5,
			T_WAREHOUSE     = 6,
			T_CITADEL_WALL  = 7,
			T_DEFENSE_TOWER = 8,
		};

	public:
		static boost::shared_ptr<const CastleBuilding> get(BuildingId buildingId);
		static boost::shared_ptr<const CastleBuilding> require(BuildingId buildingId);

	public:
		BuildingId buildingId;
		unsigned buildLimit;
		Type type;
	};

	class CastleUpgradeAbstract {
	public:
		static boost::shared_ptr<const CastleUpgradeAbstract> get(CastleBuilding::Type type, unsigned buildingLevel);
		static boost::shared_ptr<const CastleUpgradeAbstract> require(CastleBuilding::Type type, unsigned buildingLevel);

	public:
		unsigned buildingLevel;
		boost::uint64_t upgradeDuration;
		boost::container::flat_map<ResourceId, boost::uint64_t> upgradeCost;
		boost::container::flat_map<BuildingId, unsigned> prerequisite;
		boost::uint64_t destructDuration;

	protected:
		CastleUpgradeAbstract() = default;
	};

	class CastleUpgradePrimary : public CastleUpgradeAbstract {
	public:
		static boost::shared_ptr<const CastleUpgradePrimary> get(unsigned buildingLevel);
		static boost::shared_ptr<const CastleUpgradePrimary> require(unsigned buildingLevel);

	public:
		boost::uint64_t maxTerritories;
	};

	class CastleUpgradeBarracks : public CastleUpgradeAbstract {
	public:
		static boost::shared_ptr<const CastleUpgradeBarracks> get(unsigned buildingLevel);
		static boost::shared_ptr<const CastleUpgradeBarracks> require(unsigned buildingLevel);

	public:
		//
	};

	class CastleUpgradeAcademy : public CastleUpgradeAbstract {
	public:
		static boost::shared_ptr<const CastleUpgradeAcademy> get(unsigned buildingLevel);
		static boost::shared_ptr<const CastleUpgradeAcademy> require(unsigned buildingLevel);

	public:
		unsigned techLevel;
	};

	class CastleUpgradeCivilian : public CastleUpgradeAbstract {
	public:
		static boost::shared_ptr<const CastleUpgradeCivilian> get(unsigned buildingLevel);
		static boost::shared_ptr<const CastleUpgradeCivilian> require(unsigned buildingLevel);

	public:
		boost::uint64_t maxPopulation;
	};

	class CastleUpgradeWarehouse : public CastleUpgradeAbstract {
	public:
		static boost::shared_ptr<const CastleUpgradeWarehouse> get(unsigned buildingLevel);
		static boost::shared_ptr<const CastleUpgradeWarehouse> require(unsigned buildingLevel);

	public:
		boost::container::flat_map<ResourceId, boost::uint64_t> maxResourceAmount;
	};

	class CastleUpgradeCitadelWall : public CastleUpgradeAbstract {
	public:
		static boost::shared_ptr<const CastleUpgradeCitadelWall> get(unsigned buildingLevel);
		static boost::shared_ptr<const CastleUpgradeCitadelWall> require(unsigned buildingLevel);

	public:
		boost::uint64_t strength;
		boost::uint64_t armor;
	};

	class CastleUpgradeDefenseTower : public CastleUpgradeAbstract {
	public:
		static boost::shared_ptr<const CastleUpgradeDefenseTower> get(unsigned buildingLevel);
		static boost::shared_ptr<const CastleUpgradeDefenseTower> require(unsigned buildingLevel);

	public:
		boost::uint64_t firepower;
	};

	class CastleTech {
	public:
		static boost::shared_ptr<const CastleTech> get(TechId techId, unsigned techLevel);
		static boost::shared_ptr<const CastleTech> require(TechId techId, unsigned techLevel);

	public:
		std::pair<TechId, unsigned> techIdLevel;
		boost::uint64_t upgradeDuration;
		boost::container::flat_map<ResourceId, boost::uint64_t> upgradeCost;
		boost::container::flat_map<BuildingId, unsigned> prerequisite;
		boost::container::flat_map<BuildingId, unsigned> displayPrerequisite;
		boost::container::flat_map<AttributeId, double> attributes;
	};
}

}

#endif
