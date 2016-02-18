#ifndef EMPERY_CENTER_CASTLE_HPP_
#define EMPERY_CENTER_CASTLE_HPP_

#include "map_object.hpp"
#include <poseidon/cxx_util.hpp>
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
}

class PlayerSession;

class Castle : public MapObject {
public:
	enum Mission {
		MIS_NONE            = 0,
		MIS_CONSTRUCT       = 1,
		MIS_UPGRADE         = 2,
		MIS_DESTRUCT        = 3,
	};

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

	struct BattalionInfo {
		MapObjectTypeId map_object_type_id;
		std::uint64_t count;
		bool enabled;
	};

	struct BattalionProductionInfo {
		BuildingBaseId building_base_id;
		MapObjectTypeId map_object_type_id;
		std::uint64_t count;
		std::uint64_t production_duration;
		std::uint64_t production_time_begin;
		std::uint64_t production_time_end;
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
		boost::shared_ptr<MySql::Center_CastleBattalion>> m_battalions;

	boost::shared_ptr<MySql::Center_CastleBattalionProduction> m_population_production_stamps;

	boost::container::flat_map<BuildingBaseId,
		boost::shared_ptr<MySql::Center_CastleBattalionProduction>> m_battalion_production;
	bool m_locked_by_soldier_transaction = false;

	// 非持久化数据。
	double m_production_remainder = 0;
	double m_production_rate = 0;
	double m_capacity = 0;

public:
	Castle(MapObjectUuid map_object_uuid,
		AccountUuid owner_uuid, MapObjectUuid parent_object_uuid, std::string name, Coord coord, std::uint64_t created_time);
	Castle(boost::shared_ptr<MySql::Center_MapObject> obj,
		const std::vector<boost::shared_ptr<MySql::Center_MapObjectAttribute>> &attributes,
		const std::vector<boost::shared_ptr<MySql::Center_CastleBuildingBase>> &buildings,
		const std::vector<boost::shared_ptr<MySql::Center_CastleTech>> &techs,
		const std::vector<boost::shared_ptr<MySql::Center_CastleResource>> &resources,
		const std::vector<boost::shared_ptr<MySql::Center_CastleBattalion>> &battalions,
		const std::vector<boost::shared_ptr<MySql::Center_CastleBattalionProduction>> &battalion_production);
	~Castle();

public:
	void pump_status() override;
	void recalculate_attributes() override;

	double get_production_rate() const {
		return m_production_rate;
	}
	double get_capacity() const {
		return m_capacity;
	}

	void pump_production();

	void check_init_buildings();

	BuildingBaseInfo get_building_base(BuildingBaseId building_base_id) const;
	void get_all_building_bases(std::vector<BuildingBaseInfo> &ret) const;
	void get_buildings_by_id(std::vector<BuildingBaseInfo> &ret, BuildingId building_id) const;
	void get_buildings_by_type_id(std::vector<BuildingBaseInfo> &ret, BuildingTypeId type) const;
	// 如果指定地基上有任务会抛出异常。
	void create_building_mission(BuildingBaseId building_base_id, Mission mission, BuildingId building_id = BuildingId());
	void cancel_building_mission(BuildingBaseId building_base_id);
	void speed_up_building_mission(BuildingBaseId building_base_id, std::uint64_t delta_duration);

	void pump_building_status(BuildingBaseId building_base_id);
	unsigned get_building_queue_size() const;
	void synchronize_building_with_player(BuildingBaseId building_base_id, const boost::shared_ptr<PlayerSession> &session) const;

	// 各个建筑的独立接口。
	unsigned get_max_level(BuildingId building_id) const;
	unsigned get_level() const; // 领主府
	std::uint64_t get_warehouse_capacity(ResourceId resource_id) const; // 仓库
	bool is_tech_upgrade_in_progress() const; // 学院
	bool is_battalion_production_in_progress(BuildingBaseId building_base_id) const;
	std::uint64_t get_max_battalion_count() const; // 校场

	TechInfo get_tech(TechId tech_id) const;
	void get_all_techs(std::vector<TechInfo> &ret) const;
	// 同上。
	void create_tech_mission(TechId tech_id, Mission mission);
	void cancel_tech_mission(TechId tech_id);
	void speed_up_tech_mission(TechId tech_id, std::uint64_t delta_duration);

	void pump_tech_status(TechId tech_id);
	unsigned get_tech_queue_size() const;
	void synchronize_tech_with_player(TechId tech_id, const boost::shared_ptr<PlayerSession> &session) const;

	void check_init_resources();

	ResourceInfo get_resource(ResourceId resource_id) const;
	void get_all_resources(std::vector<ResourceInfo> &ret) const;

	__attribute__((__warn_unused_result__))
	ResourceId commit_resource_transaction_nothrow(const std::vector<ResourceTransactionElement> &transaction,
		const boost::function<void ()> &callback = boost::function<void ()>());
	void commit_resource_transaction(const std::vector<ResourceTransactionElement> &transaction,
		const boost::function<void ()> &callback = boost::function<void ()>());

	BattalionInfo get_battalion(MapObjectTypeId map_object_type_id) const;
	void get_all_battalions(std::vector<BattalionInfo> &ret) const;
	void enable_battalion(MapObjectTypeId map_object_type_id);

	__attribute__((__warn_unused_result__))
	MapObjectTypeId commit_soldier_transaction_nothrow(const std::vector<SoldierTransactionElement> &transaction,
		const boost::function<void ()> &callback = boost::function<void ()>());
	void commit_soldier_transaction(const std::vector<SoldierTransactionElement> &transaction,
		const boost::function<void ()> &callback = boost::function<void ()>());

	BattalionProductionInfo get_battalion_production(BuildingBaseId building_base_id) const;
	void get_all_battalion_production(std::vector<BattalionProductionInfo> &ret) const;

	void begin_battalion_production(BuildingBaseId building_base_id, MapObjectTypeId map_object_type_id, std::uint64_t count);
	void cancel_battalion_production(BuildingBaseId building_base_id);
	void speed_up_battalion_production(BuildingBaseId building_base_id, std::uint64_t delta_duration);

	void harvest_battalion(BuildingBaseId building_base_id);

	void synchronize_battalion_production_with_player(BuildingBaseId building_base_id, const boost::shared_ptr<PlayerSession> &session) const;

	void synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const;
};

inline void synchronize_castle_with_player(const boost::shared_ptr<const Castle> &castle,
	const boost::shared_ptr<PlayerSession> &session)
{
	castle->synchronize_with_player(session);
}
inline void synchronize_castle_with_player(const boost::shared_ptr<Castle> &castle,
	const boost::shared_ptr<PlayerSession> &session)
{
	castle->synchronize_with_player(session);
}

}

#endif
