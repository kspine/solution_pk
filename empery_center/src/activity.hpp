#ifndef EMPERY_CENTER_ACTIVITY_HPP_
#define EMPERY_CENTER_ACTIVITY_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <cstddef>
#include <boost/container/flat_map.hpp>
#include <boost/shared_ptr.hpp>
#include "id_types.hpp"
#include "player_session.hpp"

namespace EmperyCenter {
class Activity : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis{
public:
	Activity(std::uint64_t unique_id,std::uint64_t available_since,std::uint64_t available_until);
public:
	std::uint64_t m_unique_id;
	std::uint64_t m_available_since;
	std::uint64_t m_available_until;
public:
	virtual void  pump_status();
};

class MapActivity : public Activity{
public:
	struct MapActivityDetailInfo {
		std::uint64_t unique_id;
		std::uint64_t available_since;
		std::uint64_t available_until;
	};
public:
	MapActivityId m_current_activity;
	std::uint64_t m_next_activity_time;
	boost::container::flat_map<MapActivityId,MapActivityDetailInfo> m_activitys;
public:
	MapActivity(std::uint64_t unique_id,std::uint64_t available_since,std::uint64_t available_until);
public:
	void pump_status() override;
	void on_activity_change(MapActivityId old_ativity,MapActivityId new_activity);
	void synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const;
public:
	void set_current_activity(MapActivityId);
	MapActivityId get_current_activity();
	MapActivityDetailInfo get_activity_info(MapActivityId);
};

}

#endif

