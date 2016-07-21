#ifndef EMPERY_CENTER_CASTLE_HPP_
#define EMPERY_CENTER_CASTLE_HPP_

#include "defense_building.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/function.hpp>
#include <vector>
#include "transaction_element_fwd.hpp"

namespace EmperyCenter {

namespace MySql {
	class Center_CastleBuildingBase;
	class Center_CastleResource;
	class Center_CastleTech;
	class Center_CastleBattalion;
	class Center_CastleBattalionProduction;
	class Center_CastleWoundedSoldier;
	class Center_CastleTreatment;
}

class Castle : public DefenseBuilding {
public:
	struct BuildingBaseInfo {
		BuildingBaseId building_base_id;
		BuildingId building_id;
		unsigned building_level;
		Mission mission;
		std::uint64_t mission_duration;
		std::uint64_t mission_time_begin;
		std::uint64_t mission_time_end;
	};

	struct TechInfo {
		TechId tech_id;
		unsigned tech_level;
		Mission mission;
		std::uint64_t mission_duration;
		std::uint64_t mission_time_begin;
		std::uint64_t mission_time_end;
	};

	struct ResourceInfo {
		ResourceId resource_id;
		std::uint64_t amount;
	};

	struct SoldierInfo {
		MapObjectTypeId map_object_type_id;
		std::uint64_t count;
		bool enabled;
	};

	struct SoldierProductionInfo {
		BuildingBaseId building_base_id;
		MapObjectTypeId map_object_type_id;
		std::uint64_t count;
		std::uint64_t production_duration;
		std::uint64_t production_time_begin;
		std::uint64_t production_time_end;
	};

	struct WoundedSoldierInfo {
		MapObjectTypeId map_object_type_id;
		std::uint64_t count;
	};

	struct TreatmentInfo {
		MapObjectTypeId map_object_type_id;
		std::uint64_t count;
		std::uint64_t duration;
		std::uint64_t time_begin;
		std::uint64_t time_end;
	};

private:
	boost::container::flat_map<BuildingBaseId,
		boost::shared_ptr<MySql::Center_CastleBuildingBase>> m_buildings;
	boost::container::flat_map<TechId,
		boost::shared_ptr<MySql::Center_CastleTech>> m_techs;

	boost::container::flat_map<ResourceId,
		boost::shared_ptr<MySql::Center_CastleResource>> m_resources;
	bool m_locked_by_resource_transaction = false;

	boost::container::flat_map<MapObjectTypeId,
		boost::shared_ptr<MySql::Center_CastleBattalion>> m_soldiers;
	bool m_locked_by_soldier_transaction = false;

	boost::shared_ptr<MySql::Center_CastleBattalionProduction> m_population_production_stamps;

	boost::container::flat_map<BuildingBaseId,
		boost::shared_ptr<MySql::Center_CastleBattalionProduction>> m_soldier_production;

	boost::container::flat_map<MapObjectTypeId,
		boost::shared_ptr<MySql::Center_CastleWoundedSoldier>> m_wounded_soldiers;
	bool m_locked_by_wounded_soldier_transaction = false;

	boost::container::flat_map<MapObjectTypeId,
		boost::shared_ptr<MySql::Center_CastleTreatment>> m_treatment;

	// 非持久化数据。
	double m_population_production_remainder = 0;
	double m_population_production_rate = 0;
	double m_population_capacity = 0;

public:
	Castle(MapObjectUuid map_object_uuid, AccountUuid owner_uuid, MapObjectUuid parent_object_uuid,
		std::string name, Coord coord, std::uint64_t created_time);
	Castle(boost::shared_ptr<MySql::Center_MapObject> obj,
		const std::vector<boost::shared_ptr<MySql::Center_MapObjectAttribute>> &attributes,
		const std::vector<boost::shared_ptr<MySql::Center_MapObjectBuff>> &buffs,
		const std::vector<boost::shared_ptr<MySql::Center_DefenseBuilding>> &defense_objs,
		const std::vector<boost::shared_ptr<MySql::Center_CastleBuildingBase>> &buildings,
		const std::vector<boost::shared_ptr<MySql::Center_CastleTech>> &techs,
		const std::vector<boost::shared_ptr<MySql::Center_CastleResource>> &resources,
		const std::vector<boost::shared_ptr<MySql::Center_CastleBattalion>> &soldiers,
		const std::vector<boost::shared_ptr<MySql::Center_CastleBattalionProduction>> &soldier_production,
		const std::vector<boost::shared_ptr<MySql::Center_CastleWoundedSoldier>> &wounded_soldiers,
		const std::vector<boost::shared_ptr<MySql::Center_CastleTreatment>> &treatment);
	~Castle();

public:
	void pump_status() override;
	void recalculate_attributes(bool recursive) override;

	double get_population_production_rate() const {
		return m_population_production_rate;
	}
	double get_population_capacity() const {
		return m_population_capacity;
	}

	void pump_population_production();

	void check_init_buildings();

	BuildingBaseInfo get_building_base(BuildingBaseId building_base_id) const;
	void get_all_building_bases(std::vector<BuildingBaseInfo> &ret) const;
	void get_buildings_by_id(std::vector<BuildingBaseInfo> &ret, BuildingId building_id) const;
	void get_buildings_by_type_id(std::vector<BuildingBaseInfo> &ret, BuildingTypeId type) const;
	// 如果指定地基上有任务会抛出异常。
	void create_building_mission(BuildingBaseId building_base_id, Mission mission, std::uint64_t duration, BuildingId building_id);
	void cancel_building_mission(BuildingBaseId building_base_id);
	void speed_up_building_mission(BuildingBaseId building_base_id, std::uint64_t delta_duration);
	void forced_replace_building(BuildingBaseId building_base_id, BuildingId building_id, unsigned building_level);

	void pump_building_status(BuildingBaseId building_base_id);
	unsigned get_building_queue_size() const;
	void synchronize_building_with_player(BuildingBaseId building_base_id, const boost::shared_ptr<PlayerSession> &session) const;

	// <建筑 ID, <等级, 数量>>
	void accumulate_building_levels(boost::container::flat_map<BuildingId, boost::container::flat_map<unsigned, std::size_t>> &ret) const;

	// 各个建筑的独立接口。
	unsigned get_max_level(BuildingId building_id) const; // （通用）
	unsigned get_level() const override; // 领主府
	std::uint64_t get_warehouse_capacity(ResourceId resource_id) const; // 仓库
	std::uint64_t get_warehouse_protection(ResourceId resource_id) const; // 仓库
	bool is_tech_upgrade_in_progress() const; // 学院
	bool is_soldier_production_in_progress(BuildingBaseId building_base_id) const; // （通用）兵营
	std::uint64_t get_max_battalion_count() const; // 校场
	std::uint64_t get_max_soldier_count_bonus() const; // 校场
	std::uint64_t get_medical_tent_capacity() const; // 医疗帐篷
	bool is_treatment_in_progress() const; // 医疗帐篷

	TechInfo get_tech(TechId tech_id) const;
	void get_all_techs(std::vector<TechInfo> &ret) const;
	// 同上。
	void create_tech_mission(TechId tech_id, Mission mission, std::uint64_t duration);
	void cancel_tech_mission(TechId tech_id);
	void speed_up_tech_mission(TechId tech_id, std::uint64_t delta_duration);
	void forced_replace_tech(TechId tech_id, unsigned tech_level);

	void pump_tech_status(TechId tech_id);
	unsigned get_tech_queue_size() const;
	void synchronize_tech_with_player(TechId tech_id, const boost::shared_ptr<PlayerSession> &session) const;

	void check_init_resources();
	void check_auto_inc_resources();

	ResourceInfo get_resource(ResourceId resource_id) const;
	void get_all_resources(std::vector<ResourceInfo> &ret) const;

	__attribute__((__warn_unused_result__))
	ResourceId commit_resource_transaction_nothrow(const std::vector<ResourceTransactionElement> &transaction,
		const boost::function<void ()> &callback = boost::function<void ()>());
	void commit_resource_transaction(const std::vector<ResourceTransactionElement> &transaction,
		const boost::function<void ()> &callback = boost::function<void ()>());

	SoldierInfo get_soldier(MapObjectTypeId map_object_type_id) const;
	void get_all_soldiers(std::vector<SoldierInfo> &ret) const;
	void enable_soldier(MapObjectTypeId map_object_type_id);

	__attribute__((__warn_unused_result__))
	MapObjectTypeId commit_soldier_transaction_nothrow(const std::vector<SoldierTransactionElement> &transaction,
		const boost::function<void ()> &callback = boost::function<void ()>());
	void commit_soldier_transaction(const std::vector<SoldierTransactionElement> &transaction,
		const boost::function<void ()> &callback = boost::function<void ()>());

	SoldierProductionInfo get_soldier_production(BuildingBaseId building_base_id) const;
	void get_all_soldier_production(std::vector<SoldierProductionInfo> &ret) const;

	void begin_soldier_production(BuildingBaseId building_base_id,
		MapObjectTypeId map_object_type_id, std::uint64_t count, std::uint64_t duration);
	void cancel_soldier_production(BuildingBaseId building_base_id);
	void speed_up_soldier_production(BuildingBaseId building_base_id, std::uint64_t delta_duration);

	std::uint64_t harvest_soldier(BuildingBaseId building_base_id);
	void synchronize_soldier_production_with_player(BuildingBaseId building_base_id, const boost::shared_ptr<PlayerSession> &session) const;

	bool has_wounded_soldiers() const;
	WoundedSoldierInfo get_wounded_soldier(MapObjectTypeId map_object_type_id) const;
	void get_all_wounded_soldiers(std::vector<WoundedSoldierInfo> &ret) const;

	__attribute__((__warn_unused_result__))
	MapObjectTypeId commit_wounded_soldier_transaction_nothrow(const std::vector<WoundedSoldierTransactionElement> &transaction,
		const boost::function<void ()> &callback = boost::function<void ()>());
	void commit_wounded_soldier_transaction(const std::vector<WoundedSoldierTransactionElement> &transaction,
		const boost::function<void ()> &callback = boost::function<void ()>());

	void pump_treatment();

	void get_treatment(std::vector<TreatmentInfo> &ret) const;
	void begin_treatment(const boost::container::flat_map<MapObjectTypeId, std::uint64_t> &soldiers, std::uint64_t duration);
	void cancel_treatment();
	void speed_up_treatment(std::uint64_t delta_duration);

	void harvest_treatment();
	void synchronize_treatment_with_player(const boost::shared_ptr<PlayerSession> &session) const;

	void synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const;
};

}

#endif
