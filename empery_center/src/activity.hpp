#ifndef EMPERY_CENTER_ACTIVITY_HPP_
#define EMPERY_CENTER_ACTIVITY_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <cstddef>
#include <boost/container/flat_map.hpp>
#include <boost/shared_ptr.hpp>
#include "id_types.hpp"
#include "player_session.hpp"
#include "singletons/map_activity_accumulate_map.hpp"

namespace EmperyCenter {
class Activity : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis{
public:
	Activity(std::uint64_t unique_id,std::uint64_t available_since,std::uint64_t available_until);
	virtual ~Activity();
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
	~MapActivity();
public:
	void pump_status() override;
	void on_activity_change(MapActivityId old_ativity,MapActivityId new_activity);
	void synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const;
	bool settle_kill_soliders_activity(std::uint64_t now);
	void synchronize_kill_soliders_rank_with_player(const boost::shared_ptr<PlayerSession> &session);
public:
	void set_current_activity(MapActivityId);
	MapActivityId get_current_activity();
	MapActivityDetailInfo get_activity_info(MapActivityId);
};

class WorldActivity : public Activity {
	typedef std::pair<AccountUuid,std::uint64_t> ACCOUNT_ACCUMULATE_PAIR;
	typedef std::pair<std::uint64_t,std::vector<std::pair<AccountUuid,std::uint64_t>>> LAST_ACCOUNT_ACCUMULATE_PAIR;
public:
	bool m_expired_remove;
	boost::container::flat_map<Coord,LAST_ACCOUNT_ACCUMULATE_PAIR> m_last_world_accumulates;
public:
	WorldActivity(std::uint64_t unique_id,std::uint64_t available_since,std::uint64_t available_until);
	~WorldActivity();
public:
	void pump_status() override;
	void on_activity_change(WorldActivityId old_activity, WorldActivityId new_activity,Coord cluster_coord);
	void on_activity_expired();
	bool is_on();
	bool is_world_activity_on(Coord cluster_coord,WorldActivityId world_activity_id);
	void update_world_activity_schedule(Coord cluster_coord,WorldActivityId world_activity_id,AccountUuid account_uuid,std::uint64_t delta,bool boss_die = false);
	void synchronize_with_player(const Coord cluster_coord,AccountUuid account_uuid,const boost::shared_ptr<PlayerSession> &session) const;
	void account_accmulate_sort(const Coord cluster_coord,std::vector<std::pair<AccountUuid,std::uint64_t> > &ret);
	bool settle_world_activity(Coord cluster_coord,std::uint64_t utc_now);
	bool settle_world_activity_in_activity(Coord cluster_coord,std::uint64_t utc_now,std::vector<WorldActivityRankMap::WorldActivityRankInfo> &ret);
	void synchronize_world_rank_with_player(const Coord cluster_coord,AccountUuid account_uuid,const boost::shared_ptr<PlayerSession> &session);
	void synchronize_world_boss_with_player(const Coord cluster_coord,const boost::shared_ptr<PlayerSession> &session);
	void reward_activity(Coord cluster_coord,WorldActivityId world_activity_id,AccountUuid account_uuid);
	void reward_rank(Coord cluster_coord,AccountUuid account_uuid);
};

}

#endif

