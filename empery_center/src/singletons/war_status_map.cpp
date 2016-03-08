#include "../precompiled.hpp"
#include "war_status_map.hpp"
#include "../mmain.hpp"
#include <poseidon/singletons/timer_daemon.hpp>
#include <poseidon/multi_index_map.hpp>
#include "../msg/sc_account.hpp"
#include "player_session_map.hpp"
#include "../player_session.hpp"

namespace EmperyCenter {

namespace {
	struct WarStatusElement {
		std::pair<AccountUuid, AccountUuid> account_uuid_pair;
		AccountUuid less_account_uuid;
		AccountUuid greater_account_uuid;
		std::uint64_t expiry_time;

		WarStatusElement(AccountUuid less_account_uuid_, AccountUuid greater_account_uuid_, std::uint64_t expiry_time_)
			: account_uuid_pair(less_account_uuid_, greater_account_uuid_)
			, less_account_uuid(less_account_uuid_), greater_account_uuid(greater_account_uuid_), expiry_time(expiry_time_)
		{
			assert(less_account_uuid < greater_account_uuid);
		}
	};

	MULTI_INDEX_MAP(WarStatusContainer, WarStatusElement,
		UNIQUE_MEMBER_INDEX(account_uuid_pair)
		MULTI_MEMBER_INDEX(less_account_uuid)
		MULTI_MEMBER_INDEX(greater_account_uuid)
		MULTI_MEMBER_INDEX(expiry_time)
	)

	boost::weak_ptr<WarStatusContainer> g_war_status_map;

	void fill_status_info(WarStatusMap::StatusInfo &info, const WarStatusElement &elem){
		PROFILE_ME;

		info.less_account_uuid    = elem.less_account_uuid;
		info.greater_account_uuid = elem.greater_account_uuid;
		info.expiry_time          = elem.expiry_time;
	}
	void fill_status_msg(Msg::SC_AccountWarStatus &msg, const WarStatusElement &elem, std::uint64_t utc_now, AccountUuid self_uuid){
		PROFILE_ME;

		if(elem.less_account_uuid != self_uuid){
			msg.enemy_account_uuid = elem.less_account_uuid.str();
		} else {
			msg.enemy_account_uuid = elem.greater_account_uuid.str();
		}
		msg.expiry_duration = saturated_sub(elem.expiry_time, utc_now);
	}

	void gc_timer_proc(std::uint64_t now){
		PROFILE_ME;
		LOG_EMPERY_CENTER_TRACE("War status gc timer: now = ", now);

		const auto war_status_map = g_war_status_map.lock();
		if(!war_status_map){
			return;
		}

		for(;;){
			const auto it = war_status_map->begin<3>();
			if(it == war_status_map->end<3>()){
				break;
			}
			if(now < it->expiry_time){
				break;
			}

			war_status_map->erase<3>(it);
		}
	}

	MODULE_RAII_PRIORITY(handles, 5000){
		const auto war_status_map = boost::make_shared<WarStatusContainer>();
		g_war_status_map = war_status_map;
		handles.push(war_status_map);

		const auto gc_interval = get_config<std::uint64_t>("war_status_refresh_interval", 60000);
		auto timer = Poseidon::TimerDaemon::register_timer(0, gc_interval,
			std::bind(&gc_timer_proc, std::placeholders::_2));
		handles.push(timer);
	}
}

WarStatusMap::StatusInfo WarStatusMap::get(AccountUuid first_account_uuid, AccountUuid second_account_uuid){
	PROFILE_ME;

	AccountUuid less_account_uuid, greater_account_uuid;
	if(first_account_uuid < second_account_uuid){
		less_account_uuid    = first_account_uuid;
		greater_account_uuid = second_account_uuid;
	} else {
		less_account_uuid    = second_account_uuid;
		greater_account_uuid = first_account_uuid;
	}

	StatusInfo info = { };
	info.less_account_uuid    = less_account_uuid;
	info.greater_account_uuid = greater_account_uuid;

	const auto war_status_map = g_war_status_map.lock();
	if(!war_status_map){
		LOG_EMPERY_CENTER_WARNING("War status map is not loaded.");
		return info;
	}

	const auto it = war_status_map->find<0>(std::make_pair(less_account_uuid, greater_account_uuid));
	if(it == war_status_map->end<0>()){
		LOG_EMPERY_CENTER_DEBUG("War status not found: less_account_uuid = ", less_account_uuid, ", greater_account_uuid = ", greater_account_uuid);
		return info;
	}
	fill_status_info(info, *it);
	return info;
}
void WarStatusMap::WarStatusMap::get_all(std::vector<WarStatusMap::StatusInfo> &ret, AccountUuid account_uuid){
	PROFILE_ME;

	const auto war_status_map = g_war_status_map.lock();
	if(!war_status_map){
		LOG_EMPERY_CENTER_WARNING("War status map is not loaded.");
		return;
	}

	const auto range1 = war_status_map->equal_range<1>(account_uuid);
	const auto range2 = war_status_map->equal_range<2>(account_uuid);
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range1.first, range1.second))
	                       + static_cast<std::size_t>(std::distance(range2.first, range2.second)));
	for(auto it = range1.first; it != range1.second; ++it){
		StatusInfo info;
		fill_status_info(info, *it);
		ret.emplace_back(std::move(info));
	}
	for(auto it = range2.first; it != range2.second; ++it){
		StatusInfo info;
		fill_status_info(info, *it);
		ret.emplace_back(std::move(info));
	}
}

void WarStatusMap::set(AccountUuid first_account_uuid, AccountUuid second_account_uuid, std::uint64_t expiry_time){
	PROFILE_ME;

	if(first_account_uuid == second_account_uuid){
		LOG_EMPERY_CENTER_WARNING("Attempting to set reflexive war status? first_account_uuid = ", first_account_uuid);
		DEBUG_THROW(Exception, sslit("Attempting to set reflexive war status"));
	}

	const auto war_status_map = g_war_status_map.lock();
	if(!war_status_map){
		LOG_EMPERY_CENTER_WARNING("War status map is not loaded.");
		DEBUG_THROW(Exception, sslit("War status map is not loaded"));
	}

	AccountUuid less_account_uuid, greater_account_uuid;
	if(first_account_uuid < second_account_uuid){
		less_account_uuid    = first_account_uuid;
		greater_account_uuid = second_account_uuid;
	} else {
		less_account_uuid    = second_account_uuid;
		greater_account_uuid = first_account_uuid;
	}

	auto elem = WarStatusElement(less_account_uuid, greater_account_uuid, expiry_time);
	auto it = war_status_map->find<0>(std::make_pair(less_account_uuid, greater_account_uuid));
	if(it == war_status_map->end<0>()){
		it = war_status_map->insert<0>(std::move(elem)).first;
	} else {
		war_status_map->replace<0>(it, std::move(elem));
	}

	const auto utc_now = Poseidon::get_utc_time();

	if(less_account_uuid){
		const auto session = PlayerSessionMap::get(less_account_uuid);
		if(session){
			try {
				Msg::SC_AccountWarStatus msg;
				fill_status_msg(msg, *it, utc_now, less_account_uuid);
				session->send(msg);
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
				session->shutdown(e.what());
			}
		}
	}

	if(greater_account_uuid){
		const auto session = PlayerSessionMap::get(greater_account_uuid);
		if(session){
			try {
				Msg::SC_AccountWarStatus msg;
				fill_status_msg(msg, *it, utc_now, greater_account_uuid);
				session->send(msg);
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
				session->shutdown(e.what());
			}
		}
	}
}

void WarStatusMap::synchronize_with_player_by_account(const boost::shared_ptr<PlayerSession> &session, AccountUuid account_uuid){
	PROFILE_ME;

	const auto war_status_map = g_war_status_map.lock();
	if(!war_status_map){
		LOG_EMPERY_CENTER_WARNING("War status map is not loaded.");
		return;
	}

	const auto utc_now = Poseidon::get_utc_time();

	const auto range1 = war_status_map->equal_range<1>(account_uuid);
	for(auto it = range1.first; it != range1.second; ++it){
		Msg::SC_AccountWarStatus msg;
		fill_status_msg(msg, *it, utc_now, account_uuid);
		session->send(msg);
	}
	const auto range2 = war_status_map->equal_range<2>(account_uuid);
	for(auto it = range2.first; it != range2.second; ++it){
		Msg::SC_AccountWarStatus msg;
		fill_status_msg(msg, *it, utc_now, account_uuid);
		session->send(msg);
	}
}

}
