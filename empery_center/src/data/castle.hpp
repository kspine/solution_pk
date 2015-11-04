#ifndef EMPERY_CENTER_DATA_SNG_LEVEL_HPP_
#define EMPERY_CENTER_DATA_SNG_LEVEL_HPP_

#include "common.hpp"
#include <boost/container/flat_set.hpp>
#include <boost/container/flat_map.hpp>

namespace EmperyCenter {

namespace Data {
	class CastleBuildingBase {
	public:
		static boost::shared_ptr<const CastleBuildingBase> get(unsigned baseIndex);
		static boost::shared_ptr<const CastleBuildingBase> require(unsigned baseIndex);

	public:
		unsigned baseIndex;
		boost::container::flat_set<BuildingId> buildingsAllowed;
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
		bool destructible;
	};

	class CastleUpgradingPrimary {
	public:
		static boost::shared_ptr<const CastleUpgradingPrimary> get(unsigned level);
		static boost::shared_ptr<const CastleUpgradingPrimary> require(unsigned level);

	public:
		unsigned level;
		boost::uint64_t upgradeDuration;
		boost::container::flat_map<ResourceId, boost::uint64_t> upgradeCost;
		boost::container::flat_map<BuildingId, unsigned> prerequisite;

		boost::uint64_t maxTerritories;
	};

	class CastleUpgradingBarracks {
	public:
		static boost::shared_ptr<const CastleUpgradingBarracks> get(unsigned level);
		static boost::shared_ptr<const CastleUpgradingBarracks> require(unsigned level);

	public:
		unsigned level;
		boost::uint64_t upgradeDuration;
		boost::container::flat_map<ResourceId, boost::uint64_t> upgradeCost;
		boost::container::flat_map<BuildingId, unsigned> prerequisite;

		//
	};

	class CastleUpgradingAcademy {
	public:
		static boost::shared_ptr<const CastleUpgradingAcademy> get(unsigned level);
		static boost::shared_ptr<const CastleUpgradingAcademy> require(unsigned level);

	public:
		unsigned level;
		boost::uint64_t upgradeDuration;
		boost::container::flat_map<ResourceId, boost::uint64_t> upgradeCost;
		boost::container::flat_map<BuildingId, unsigned> prerequisite;

		unsigned techLevel;
	};

	class CastleUpgradingCivilian {
	public:
		static boost::shared_ptr<const CastleUpgradingCivilian> get(unsigned level);
		static boost::shared_ptr<const CastleUpgradingCivilian> require(unsigned level);

	public:
		unsigned level;
		boost::uint64_t upgradeDuration;
		boost::container::flat_map<ResourceId, boost::uint64_t> upgradeCost;
		boost::container::flat_map<BuildingId, unsigned> prerequisite;

		boost::uint64_t maxPopulation;
	};

	class CastleUpgradingWarehouse {
	public:
		static boost::shared_ptr<const CastleUpgradingWarehouse> get(unsigned level);
		static boost::shared_ptr<const CastleUpgradingWarehouse> require(unsigned level);

	public:
		unsigned level;
		boost::uint64_t upgradeDuration;
		boost::container::flat_map<ResourceId, boost::uint64_t> upgradeCost;
		boost::container::flat_map<BuildingId, unsigned> prerequisite;

		boost::container::flat_map<ResourceId, boost::uint64_t> maxResourceAmount;
	};

	class CastleUpgradingCitadelWall {
	public:
		static boost::shared_ptr<const CastleUpgradingCitadelWall> get(unsigned level);
		static boost::shared_ptr<const CastleUpgradingCitadelWall> require(unsigned level);

	public:
		unsigned level;
		boost::uint64_t upgradeDuration;
		boost::container::flat_map<ResourceId, boost::uint64_t> upgradeCost;
		boost::container::flat_map<BuildingId, unsigned> prerequisite;

		boost::uint64_t strength;
		boost::uint64_t armor;
	};

	class CastleUpgradingDefenseTower {
	public:
		static boost::shared_ptr<const CastleUpgradingDefenseTower> get(unsigned level);
		static boost::shared_ptr<const CastleUpgradingDefenseTower> require(unsigned level);

	public:
		unsigned level;
		boost::uint64_t upgradeDuration;
		boost::container::flat_map<ResourceId, boost::uint64_t> upgradeCost;
		boost::container::flat_map<BuildingId, unsigned> prerequisite;

		boost::uint64_t firepower;
	};
}

}

#endif
