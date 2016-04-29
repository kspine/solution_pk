#ifndef EMPERY_CENTER_DEFENSE_BUILDING_HPP_
#define EMPERY_CENTER_DEFENSE_BUILDING_HPP_

#include "map_object.hpp"
#include <boost/shared_ptr.hpp>
#include <poseidon/fwd.hpp>

namespace EmperyCenter {

namespace MySql {
	class Center_DefenseBuilding;
}

class DefenseBuilding : public MapObject {
public:
	enum Mission {
		MIS_NONE       = 0,
		MIS_CONSTRUCT  = 1,
		MIS_UPGRADE    = 2,
		MIS_DESTRUCT   = 3,
	};

private:
	const boost::shared_ptr<MySql::Center_DefenseBuilding> m_defense_obj;

public:
	DefenseBuilding(MapObjectUuid map_object_uuid, MapObjectTypeId map_object_type_id, AccountUuid owner_uuid,
		MapObjectUuid parent_object_uuid, std::string name, Coord coord, std::uint64_t created_time);
	DefenseBuilding(boost::shared_ptr<MySql::Center_MapObject> obj,
		const std::vector<boost::shared_ptr<MySql::Center_MapObjectAttribute>> &attributes,
		boost::shared_ptr<MySql::Center_DefenseBuilding> defense_obj);
	~DefenseBuilding();

protected:
	void synchronize_with_player_additional(const boost::shared_ptr<PlayerSession> &session) const override;

public:
	void pump_status() override;
	void recalculate_attributes() override;

	unsigned get_building_level() const;
	Mission get_mission() const;
	std::uint64_t get_mission_duration() const;
	std::uint64_t get_mission_time_begin() const;
	std::uint64_t get_mission_time_end() const;

	void create_mission(Mission mission, std::uint64_t duration);
	void cancel_mission();
	void speed_up_mission(std::uint64_t delta_duration);
	void forced_replace_level(unsigned building_level);

	MapObjectUuid get_garrisoning_object_uuid() const;
	void set_garrisoning_object_uuid(MapObjectUuid garrisoning_object_uuid);

	void synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const;
};

inline void synchronize_defense_building_with_player(const boost::shared_ptr<const DefenseBuilding> &defense_building,
	const boost::shared_ptr<PlayerSession> &session)
{
	defense_building->synchronize_with_player(session);
}
inline void synchronize_defense_building_with_player(const boost::shared_ptr<DefenseBuilding> &defense_building,
	const boost::shared_ptr<PlayerSession> &session)
{
	defense_building->synchronize_with_player(session);
}

}

#endif
