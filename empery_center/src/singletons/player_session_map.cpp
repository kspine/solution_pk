#include "../precompiled.hpp"
#include "player_session_map.hpp"
#include "../mmain.hpp"
#include <poseidon/singletons/timer_daemon.hpp>
#include <poseidon/multi_index_map.hpp>
#include <poseidon/ip_port.hpp>
#include "account_map.hpp"
#include "../account.hpp"
#include "../account_attribute_ids.hpp"
#include "../msg/kill.hpp"
#include "../events/account.hpp"
#include "../player_session.hpp"

namespace EmperyCenter {

namespace {
	struct SessionElement {
		std::uint64_t online_since;

		AccountUuid account_uuid;
		boost::weak_ptr<PlayerSession> weak_session;

		SessionElement(AccountUuid account_uuid_, boost::weak_ptr<PlayerSession> weak_session_)
			: online_since(Poseidon::get_fast_mono_clock())
			, account_uuid(account_uuid_), weak_session(std::move(weak_session_))
		{
		}
	};

	MULTI_INDEX_MAP(PlayerSessionContainer, SessionElement,
		UNIQUE_MEMBER_INDEX(account_uuid)
		UNIQUE_MEMBER_INDEX(weak_session)
	)

	boost::shared_ptr<PlayerSessionContainer> g_session_map;

	using SessionIterator = typename std::decay<decltype(g_session_map->begin<1>())>::type;

	SessionIterator really_erase_session(SessionIterator it, std::uint64_t now, std::uint64_t utc_now){
		PROFILE_ME;

		const auto account_uuid    = it->account_uuid;
		const auto online_duration = now - it->online_since;

		const auto next = g_session_map->erase<1>(it);

		try {
			LOG_EMPERY_CENTER_DEBUG("Player goes offline: account_uuid = ", account_uuid, ", online_duration = ", online_duration);
			Poseidon::async_raise_event(boost::make_shared<Events::AccountLoggedOut>(account_uuid, online_duration));
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
		}

		try {
			const auto account = AccountMap::require(account_uuid);

			boost::container::flat_map<AccountAttributeId, std::string> modifiers;
			modifiers[AccountAttributeIds::ID_LAST_LOGGED_OUT_TIME] = boost::lexical_cast<std::string>(utc_now);
			account->set_attributes(std::move(modifiers));
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
		}

		return next;
	}

	void gc_timer_proc(std::uint64_t now){
		PROFILE_ME;
		LOG_EMPERY_CENTER_TRACE("Session gc timer: now = ", now);

		const auto utc_now = Poseidon::get_utc_time();

		auto it = g_session_map->begin<1>();
		while(it != g_session_map->end<1>()){
			const auto session = it->weak_session.lock();
			if(session){
				++it;
				continue;
			}
			it = really_erase_session(it, now, utc_now);
		}
	}

	void statistic_timer_proc(std::uint64_t now, std::uint64_t period){
		PROFILE_ME;
		LOG_EMPERY_CENTER_TRACE("Session statistic timer: now = ", now, ", period = ", period);

		Poseidon::async_raise_event(boost::make_shared<Events::AccountNumberOnline>(period, g_session_map->size()));
	}

	MODULE_RAII_PRIORITY(handles, 8000){
		const auto session_map = boost::make_shared<PlayerSessionContainer>();
		g_session_map = session_map;
		handles.push(session_map);

		const auto gc_interval = get_config<std::uint64_t>("player_session_gc_delay", 1000);
		auto timer = Poseidon::TimerDaemon::register_timer(0, gc_interval,
			std::bind(&gc_timer_proc, std::placeholders::_2));
		handles.push(timer);

		const auto statistic_interval = get_config<std::uint64_t>("account_online_statistic_interval", 60000);
		timer = Poseidon::TimerDaemon::register_timer(0, statistic_interval,
			std::bind(&statistic_timer_proc, std::placeholders::_2, std::placeholders::_3));
		handles.push(timer);
	}
}

boost::shared_ptr<PlayerSession> PlayerSessionMap::get(AccountUuid account_uuid){
	PROFILE_ME;

	const auto it = g_session_map->find<0>(account_uuid);
	if(it == g_session_map->end<0>()){
		return { };
	}
	auto session = it->weak_session.lock();
	if(!session){
		return { };
	}
	if(session->has_been_shutdown_write()){
		return { };
	}
	return session;
}

AccountUuid PlayerSessionMap::get_account_uuid(const boost::weak_ptr<PlayerSession> &weak_session){
	PROFILE_ME;

	const auto it = g_session_map->find<1>(weak_session);
	if(it == g_session_map->end<1>()){
		return { };
	}
	return it->account_uuid;
}

void PlayerSessionMap::add(AccountUuid account_uuid, const boost::shared_ptr<PlayerSession> &session){
	PROFILE_ME;

	const auto it = g_session_map->find<1>(session);
	if(it != g_session_map->end<1>()){
		LOG_EMPERY_CENTER_WARNING("Session added by another account: account_uuid = ", it->account_uuid);
		DEBUG_THROW(Exception, sslit("Duplicate session"));
	}

	const auto account = AccountMap::require(account_uuid);
	const auto utc_now = Poseidon::get_utc_time();

	for(;;){
		const auto result = g_session_map->insert(SessionElement(account_uuid, session));
		if(result.second){
			// 新会话。
			try {
				boost::container::flat_map<AccountAttributeId, std::string> modifiers;
				modifiers.reserve(2);
				modifiers[AccountAttributeIds::ID_LAST_LOGGED_IN_TIME]  = boost::lexical_cast<std::string>(utc_now);
				modifiers[AccountAttributeIds::ID_LAST_LOGGED_OUT_TIME] = std::string();
				account->set_attributes(std::move(modifiers));
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_ERROR("std::exception thrown: account_uuid = ", account_uuid, ", what = ", e.what());
				g_session_map->erase(result.first);
				session->shutdown(e.what());
				throw;
			}

			const auto remote_ip = std::string(session->get_remote_info().ip.get());
			LOG_EMPERY_CENTER_DEBUG("Player goes online: account_uuid = ", account_uuid, ", remote_ip = ", remote_ip);
			Poseidon::async_raise_event(boost::make_shared<Events::AccountLoggedIn>(account_uuid, remote_ip));
			break;
		}
		const auto other_session = result.first->weak_session.lock();
		if(other_session == session){
			// 会话已存在。
			break;
		}
		if(other_session){
			other_session->shutdown(Msg::KILL_SESSION_GHOSTED, { });
			LOG_EMPERY_CENTER_DEBUG("Session ghosted: account_uuid = ", account_uuid);
		}
		g_session_map->erase(result.first);
	}
}
void PlayerSessionMap::remove(const boost::weak_ptr<PlayerSession> &weak_session) noexcept {
	PROFILE_ME;

	const auto session = weak_session.lock();
	if(session){
		session->shutdown(Msg::KILL_OPERATOR_COMMAND, "");
	}

	const auto it = g_session_map->find<1>(weak_session);
	if(it == g_session_map->end<1>()){
		return;
	}

	const auto now     = Poseidon::get_fast_mono_clock();
	const auto utc_now = Poseidon::get_utc_time();
	really_erase_session(it, now, utc_now);
}

void PlayerSessionMap::get_all(std::vector<std::pair<AccountUuid, boost::shared_ptr<PlayerSession>>> &ret){
	PROFILE_ME;

	ret.reserve(ret.size() + g_session_map->size());
	for(auto it = g_session_map->begin(); it != g_session_map->end(); ++it){
		auto session = it->weak_session.lock();
		if(!session){
			continue;
		}
		ret.emplace_back(it->account_uuid, std::move(session));
	}
}
void PlayerSessionMap::clear(const char *reason) noexcept {
	PROFILE_ME;

	auto session_map = std::move(*g_session_map);
	g_session_map->clear();

	for(auto it = session_map.begin(); it != session_map.end(); ++it){
		auto session = it->weak_session.lock();
		if(!session){
			continue;
		}
		session->shutdown(reason);
	}
}
void PlayerSessionMap::clear(int code, const char *reason) noexcept {
	PROFILE_ME;

	auto session_map = std::move(*g_session_map);
	g_session_map->clear();

	for(auto it = session_map.begin(); it != session_map.end(); ++it){
		auto session = it->weak_session.lock();
		if(!session){
			continue;
		}
		session->shutdown(code, reason);
	}
}

}
