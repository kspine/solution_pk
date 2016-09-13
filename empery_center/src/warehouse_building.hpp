#ifndef EMPERY_CENTER_WAREHOUSE_BUILDING_HPP_
#define EMPERY_CENTER_WAREHOUSE_BUILDING_HPP_

#include "map_object.hpp"
#include <boost/shared_ptr.hpp>
#include <poseidon/fwd.hpp>

namespace EmperyCenter {

namespace MySql {
	class Center_WarehouseBuilding;
}

class WarehouseBuilding : public MapObject {
public:
	enum Mission {
		MIS_NONE       = 0,
		MIS_CONSTRUCT  = 1,
		MIS_UPGRADE    = 2,
		MIS_DESTRUCT   = 3,
		MIS_GARRISON   = 4,
		MIS_EVICT      = 5,
		MIS_OPENCD	   = 6,
		MIS_OPENEFFECT = 7,
	};

private:
	const boost::shared_ptr<MySql::Center_WarehouseBuilding> m_defense_obj;

	// 非持久化数据。
	double m_self_healing_remainder = 0;

	unsigned int  m_buffid = 0;

	// 最大血量
	std::int64_t m_maxHp = 0;

public:
	WarehouseBuilding(MapObjectUuid map_object_uuid, MapObjectTypeId map_object_type_id, AccountUuid owner_uuid,
		MapObjectUuid parent_object_uuid, std::string name, Coord coord, std::uint64_t created_time,LegionUuid legion_uuid);
	WarehouseBuilding(boost::shared_ptr<MySql::Center_MapObject> obj,
		const std::vector<boost::shared_ptr<MySql::Center_MapObjectAttribute>> &attributes,
		const std::vector<boost::shared_ptr<MySql::Center_MapObjectBuff>> &buffs,
		const std::vector<boost::shared_ptr<MySql::Center_WarehouseBuilding>> &defense_objs);
	~WarehouseBuilding();

protected:
	void synchronize_with_player_additional(const boost::shared_ptr<PlayerSession> &session) const override;

public:
	void pump_status() override;
	void recalculate_attributes(bool recursive) override;

	bool is_protectable() const override;

	LegionUuid get_legion_uuid() const;

	virtual unsigned get_level() const;
	WarehouseBuilding::Mission get_mission() const;
	std::uint64_t get_mission_duration() const;
	std::uint64_t get_mission_time_begin() const;
	std::uint64_t get_mission_time_end() const;
	std::uint64_t get_cd_time();
	std::int64_t get_max_hp();
	std::uint64_t get_output_amount();
	std::uint64_t get_output_type();
	std::uint64_t get_effect_time();

	void create_mission(WarehouseBuilding::Mission mission, std::uint64_t duration, MapObjectUuid garrisoning_object_uuid);
	void cancel_mission();
	void speed_up_mission(std::uint64_t delta_duration);
	void forced_replace_level(unsigned building_level);

	MapObjectUuid get_garrisoning_object_uuid() const;

	void self_heal();

	void synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const;
	// 血量改变，传入当前血量
	void on_hp_change(std::int64_t curhp);
	// 开启
	void open(unsigned output_type,unsigned amount,std::uint64_t effect_time,std::uint64_t  opencd_time);
	// 改变血量和最大血量
	void set_maxhp(std::uint64_t maxhp);
	// 采集矿井资源
	std::uint64_t harvest(const boost::shared_ptr<MapObject> &harvester, double amount_to_harvest, bool saturated = false);
	// 设置剩余采集量
	void set_output_amount(std::uint64_t amount);
};

}

#endif
