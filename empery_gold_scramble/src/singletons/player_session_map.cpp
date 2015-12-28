#include "../precompiled.hpp"
#include "player_session_map.hpp"
#include <poseidon/multi_index_map.hpp>
#include <poseidon/cbpp/control_message.hpp>
#include <poseidon/ip_port.hpp>
#include "../msg/kill.hpp"
#include "../player_session.hpp"

namespace EmperyGoldScramble {

namespace {
	struct SessionElement {
		std::string login_name;
		std::string nick;
		boost::weak_ptr<PlayerSession> weak_session;

		std::uint64_t online_since;

		SessionElement(const std::string &login_name_, const std::string &nick_, boost::weak_ptr<PlayerSession> weak_session_)
			: login_name(std::move(login_name_)), nick(std::move(nick_)), weak_session(std::move(weak_session_))
			, online_since(Poseidon::get_fast_mono_clock())
		{
		}
	};

	MULTI_INDEX_MAP(PlayerSessionMapContainer, SessionElement,
		UNIQUE_MEMBER_INDEX(login_name)
		MULTI_MEMBER_INDEX(nick)
		UNIQUE_MEMBER_INDEX(weak_session)
	)

	boost::shared_ptr<PlayerSessionMapContainer> g_session_map;

	MODULE_RAII_PRIORITY(handles, 8000){
		const auto session_map = boost::make_shared<PlayerSessionMapContainer>();
		g_session_map = session_map;
		handles.push(session_map);
	}
}

boost::shared_ptr<PlayerSession> PlayerSessionMap::get(const std::string &login_name){
	PROFILE_ME;

	const auto it = g_session_map->find<0>(login_name);
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

std::string PlayerSessionMap::get_login_name(const boost::weak_ptr<PlayerSession> &weak_session){
	PROFILE_ME;

	const auto it = g_session_map->find<2>(weak_session);
	if(it == g_session_map->end<2>()){
		return { };
	}
	return it->login_name;
}
std::string PlayerSessionMap::require_login_name(const boost::weak_ptr<PlayerSession> &weak_session){
	PROFILE_ME;

	auto ret = get_login_name(weak_session);
	if(ret.empty()){
		DEBUG_THROW(Exception, sslit("Session not found"));
	}
	return ret;
}

std::string PlayerSessionMap::get_nick(const boost::weak_ptr<PlayerSession> &weak_session){
	PROFILE_ME;

	const auto it = g_session_map->find<2>(weak_session);
	if(it == g_session_map->end<2>()){
		return { };
	}
	return it->nick;
}
std::string PlayerSessionMap::require_nick(const boost::weak_ptr<PlayerSession> &weak_session){
	PROFILE_ME;

	auto ret = get_nick(weak_session);
	if(ret.empty()){
		DEBUG_THROW(Exception, sslit("Session not found"));
	}
	return ret;
}

void PlayerSessionMap::add(const std::string &login_name, const std::string &nick, const boost::shared_ptr<PlayerSession> &session){
	PROFILE_ME;

	const auto it = g_session_map->find<2>(session);
	if(it != g_session_map->end<2>()){
		LOG_EMPERY_GOLD_SCRAMBLE_WARNING("Session added by another account: login_name = ", it->login_name);
		DEBUG_THROW(Exception, sslit("Duplicate session"));
	}

	for(;;){
		const auto result = g_session_map->insert(SessionElement(login_name, nick, session));
		if(result.second){
			const auto remote_ip = std::string(session->get_remote_info().ip.get());
			LOG_EMPERY_GOLD_SCRAMBLE_INFO("Player goes online: login_name = ", login_name, ", remote_ip = ", remote_ip);
			break;
		}
		const auto other_session = result.first->weak_session.lock();
		if(other_session == session){
			// 会话已存在。
			break;
		}
		if(other_session){
			other_session->shutdown(Msg::KILL_SESSION_GHOSTED, { });
			LOG_EMPERY_GOLD_SCRAMBLE_INFO("Session ghosted: login_name = ", login_name);
		}
		g_session_map->erase(result.first);
	}
}
void PlayerSessionMap::remove(const boost::weak_ptr<PlayerSession> &weak_session) noexcept {
	PROFILE_ME;

	const auto it = g_session_map->find<2>(weak_session);
	if(it == g_session_map->end<2>()){
		return;
	}

	const auto session = weak_session.lock();
	if(session){
		session->shutdown(Msg::KILL_OPERATOR_COMMAND, { });
	}

	g_session_map->erase<2>(it);
}

void PlayerSessionMap::get_all(boost::container::flat_map<std::string, boost::shared_ptr<PlayerSession>> &ret){
	PROFILE_ME;

	ret.reserve(ret.size() + g_session_map->size());
	for(auto it = g_session_map->begin(); it != g_session_map->end(); ++it){
		auto session = it->weak_session.lock();
		if(!session){
			continue;
		}
		ret.emplace(it->login_name, std::move(session));
	}
}

}
