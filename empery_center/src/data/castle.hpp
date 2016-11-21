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
		BuildingId init_building_id_override;
		boost::container::flat_map<BuildingId, unsigned> operation_prerequisite;
	};

	class CastleBuilding {
	public:
		static boost::shared_ptr<const CastleBuilding> get(BuildingId building_id);
		static boost::shared_ptr<const CastleBuilding> require(BuildingId building_id);

	public:
		BuildingId building_id;
		unsigned build_limit;
		BuildingTypeId type;
	};

	class CastleUpgradeAbstract {
	public:
		static boost::shared_ptr<const CastleUpgradeAbstract> get(BuildingTypeId type, unsigned building_level);
		static boost::shared_ptr<const CastleUpgradeAbstract> require(BuildingTypeId type, unsigned building_level);

	public:
		unsigned building_level;
		double upgrade_duration;
		boost::container::flat_map<ResourceId, std::uint64_t> upgrade_cost;
		boost::container::flat_map<BuildingId, unsigned> prerequisite;
		std::uint64_t prosperity_points;
		double destruct_duration;
	};

	class CastleUpgradeAddonAbstract : public CastleUpgradeAbstract {
	public:
		static boost::shared_ptr<const CastleUpgradeAddonAbstract> get(BuildingTypeId type, unsigned building_level);
		static boost::shared_ptr<const CastleUpgradeAddonAbstract> require(BuildingTypeId type, unsigned building_level);

	public:
		boost::container::flat_map<AttributeId, double> attributes;
	};

	class CastleUpgradeBootCamp : public CastleUpgradeAddonAbstract {
	public:
		static boost::shared_ptr<const CastleUpgradeBootCamp> get(unsigned building_level);
		static boost::shared_ptr<const CastleUpgradeBootCamp> require(unsigned building_level);

	public:
		//
	};

	class CastleUpgradePrimary : public CastleUpgradeAbstract {
	public:
		static boost::shared_ptr<const CastleUpgradePrimary> get(unsigned building_level);
		static boost::shared_ptr<const CastleUpgradePrimary> require(unsigned building_level);

	public:
		std::uint64_t max_map_cell_count;
		std::uint64_t max_map_cell_distance;
		std::uint64_t max_immigrant_group_count;
		std::uint64_t max_defense_buildings;
		boost::container::flat_map<ResourceId, std::uint64_t> protection_cost;
		std::uint64_t max_occupied_map_cells;
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
		boost::container::flat_map<ResourceId, std::uint64_t> protected_resource_amounts;
	};

	class CastleUpgradeCitadelWall : public CastleUpgradeAddonAbstract {
	public:
		static boost::shared_ptr<const CastleUpgradeCitadelWall> get(unsigned building_level);
		static boost::shared_ptr<const CastleUpgradeCitadelWall> require(unsigned building_level);

	public:
		//
	};

	class CastleUpgradeDefenseTower : public CastleUpgradeAddonAbstract {
	public:
		static boost::shared_ptr<const CastleUpgradeDefenseTower> get(unsigned building_level);
		static boost::shared_ptr<const CastleUpgradeDefenseTower> require(unsigned building_level);

	public:
		//
	};

	class CastleUpgradeParadeGround : public CastleUpgradeAbstract {
	public:
		static boost::shared_ptr<const CastleUpgradeParadeGround> get(unsigned building_level);
		static boost::shared_ptr<const CastleUpgradeParadeGround> require(unsigned building_level);

	public:
		std::uint64_t max_battalion_count;
		std::uint64_t max_soldier_count_bonus;
	};

	class CastleUpgradeMedicalTent : public CastleUpgradeAddonAbstract {
	public:
		static boost::shared_ptr<const CastleUpgradeMedicalTent> get(unsigned building_level);
		static boost::shared_ptr<const CastleUpgradeMedicalTent> require(unsigned building_level);

	public:
		std::uint64_t capacity;
	};

	class CastleUpgradeHarvestWorkshop : public CastleUpgradeAddonAbstract {
	public:
		static boost::shared_ptr<const CastleUpgradeHarvestWorkshop> get(unsigned building_level);
		static boost::shared_ptr<const CastleUpgradeHarvestWorkshop> require(unsigned building_level);

	public:
		//
	};

	class CastleUpgradeWarWorkshop : public CastleUpgradeAddonAbstract {
	public:
		static boost::shared_ptr<const CastleUpgradeWarWorkshop> get(unsigned building_level);
		static boost::shared_ptr<const CastleUpgradeWarWorkshop> require(unsigned building_level);

	public:
		//
	};

	class CastleUpgradeTree : public CastleUpgradeAbstract {
	public:
		static boost::shared_ptr<const CastleUpgradeTree> get(unsigned building_level);
		static boost::shared_ptr<const CastleUpgradeTree> require(unsigned building_level);

	public:
		//
	};
	
	class CastleUpgradeForge : public CastleUpgradeAbstract {
	public:
		static boost::shared_ptr<const CastleUpgradeForge> get(unsigned building_level);
		static boost::shared_ptr<const CastleUpgradeForge> require(unsigned building_level);

	public:
		//
	};

	class CastleTech {
	public:
		static boost::shared_ptr<const CastleTech> get(TechId tech_id, unsigned tech_level);
		static boost::shared_ptr<const CastleTech> require(TechId tech_id, unsigned tech_level);

		static void get_by_era(std::vector<boost::shared_ptr<const CastleTech>> &ret, unsigned era);

	public:
		std::pair<TechId, unsigned> tech_id_level;
		unsigned tech_era;
		double upgrade_duration;
		boost::container::flat_map<ResourceId, std::uint64_t> upgrade_cost;
		boost::container::flat_map<BuildingId, unsigned> prerequisite;
		boost::container::flat_map<BuildingId, unsigned> display_prerequisite;
		boost::container::flat_map<TechId, unsigned> tech_prerequisite;
		boost::container::flat_map<AttributeId, double> attributes;
	};

	class CastleResource {
	public:
		enum AutoIncType {
			AIT_NONE     = 0,
			AIT_HOURLY   = 1,
			AIT_DAILY    = 2,
			AIT_WEEKLY   = 3,
			AIT_PERIODIC = 4,
		};

	public:
		static boost::shared_ptr<const CastleResource> get(ResourceId resource_id);
		static boost::shared_ptr<const CastleResource> require(ResourceId resource_id);

		static boost::shared_ptr<const CastleResource> get_by_locked_resource_id(ResourceId locked_resource_id);
		static boost::shared_ptr<const CastleResource> require_by_locked_resource_id(ResourceId locked_resource_id);

		static void get_all(std::vector<boost::shared_ptr<const CastleResource>> &ret);

		static boost::shared_ptr<const CastleResource> get_by_carried_attribute_id(AttributeId attribute_id);

		static void get_init(std::vector<boost::shared_ptr<const CastleResource>> &ret);
		static void get_auto_inc(std::vector<boost::shared_ptr<const CastleResource>> &ret);

	public:
		ResourceId resource_id;

		std::uint64_t init_amount;
		bool producible;

		ResourceId locked_resource_id;
		ItemId undeployed_item_id;

		AttributeId carried_attribute_id;
		AttributeId carried_alt_attribute_id;
		double unit_weight;

		AutoIncType auto_inc_type;
		std::uint64_t auto_inc_offset;
		std::int64_t auto_inc_step;
		std::uint64_t auto_inc_bound;

		ResourceId resource_token_id;
	};
}

}

#endif
