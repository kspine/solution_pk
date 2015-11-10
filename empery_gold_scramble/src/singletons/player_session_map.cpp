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
		std::string loginName;
		std::string nick;
		boost::weak_ptr<PlayerSession> weakSession;

		boost::uint64_t onlineSince;

		SessionElement(const std::string &loginName_, const std::string &nick_, boost::weak_ptr<PlayerSession> weakSession_)
			: loginName(std::move(loginName_)), nick(std::move(nick_)), weakSession(std::move(weakSession_))
			, onlineSince(Poseidon::getFastMonoClock())
		{
		}
	};

	MULTI_INDEX_MAP(PlayerSessionMapDelegator, SessionElement,
		UNIQUE_MEMBER_INDEX(loginName)
		MULTI_MEMBER_INDEX(nick)
		UNIQUE_MEMBER_INDEX(weakSession)
	)

	boost::shared_ptr<PlayerSessionMapDelegator> g_sessionMap;

	MODULE_RAII_PRIORITY(handles, 8000){
		const auto sessionMap = boost::make_shared<PlayerSessionMapDelegator>();
		g_sessionMap = sessionMap;
		handles.push(sessionMap);
	}
}

boost::shared_ptr<PlayerSession> PlayerSessionMap::get(const std::string &loginName){
	PROFILE_ME;

	const auto it = g_sessionMap->find<0>(loginName);
	if(it == g_sessionMap->end<0>()){
		return { };
	}
	auto session = it->weakSession.lock();
	if(!session){
		return { };
	}
	if(session->hasBeenShutdownWrite()){
		return { };
	}
	return session;
}

std::string PlayerSessionMap::getLoginName(const boost::weak_ptr<PlayerSession> &weakSession){
	PROFILE_ME;

	const auto it = g_sessionMap->find<2>(weakSession);
	if(it == g_sessionMap->end<2>()){
		return { };
	}
	return it->loginName;
}
std::string PlayerSessionMap::requireLoginName(const boost::weak_ptr<PlayerSession> &weakSession){
	PROFILE_ME;

	auto ret = getLoginName(weakSession);
	if(ret.empty()){
		DEBUG_THROW(Exception, sslit("Session not found"));
	}
	return ret;
}

std::string PlayerSessionMap::getNick(const boost::weak_ptr<PlayerSession> &weakSession){
	PROFILE_ME;

	const auto it = g_sessionMap->find<2>(weakSession);
	if(it == g_sessionMap->end<2>()){
		return { };
	}
	return it->nick;
}
std::string PlayerSessionMap::requireNick(const boost::weak_ptr<PlayerSession> &weakSession){
	PROFILE_ME;

	auto ret = getNick(weakSession);
	if(ret.empty()){
		DEBUG_THROW(Exception, sslit("Session not found"));
	}
	return ret;
}

void PlayerSessionMap::add(const std::string &loginName, const std::string &nick, const boost::shared_ptr<PlayerSession> &session){
	PROFILE_ME;

	const auto it = g_sessionMap->find<2>(session);
	if(it != g_sessionMap->end<2>()){
		LOG_EMPERY_GOLD_SCRAMBLE_WARNING("Session added by another account: loginName = ", it->loginName);
		DEBUG_THROW(Exception, sslit("Duplicate session"));
	}

	for(;;){
		const auto result = g_sessionMap->insert(SessionElement(loginName, nick, session));
		if(result.second){
			const auto remoteIp = std::string(session->getRemoteInfo().ip.get());
			LOG_EMPERY_GOLD_SCRAMBLE_INFO("Player goes online: loginName = ", loginName, ", remoteIp = ", remoteIp);
			break;
		}
		const auto otherSession = result.first->weakSession.lock();
		if(otherSession == session){
			// 会话已存在。
			break;
		}
		if(otherSession){
			otherSession->shutdown(Msg::KILL_SESSION_GHOSTED, { });
			LOG_EMPERY_GOLD_SCRAMBLE_INFO("Session ghosted: loginName = ", loginName);
		}
		g_sessionMap->erase(result.first);
	}
}
void PlayerSessionMap::remove(const boost::weak_ptr<PlayerSession> &weakSession) noexcept {
	PROFILE_ME;

	const auto it = g_sessionMap->find<2>(weakSession);
	if(it == g_sessionMap->end<2>()){
		return;
	}

	const auto session = weakSession.lock();
	if(session){
		session->shutdown(Msg::KILL_OPERATOR_COMMAND, { });
	}

	g_sessionMap->erase<2>(it);
}

void PlayerSessionMap::getAll(boost::container::flat_map<std::string, boost::shared_ptr<PlayerSession>> &ret){
	PROFILE_ME;

	ret.reserve(ret.size() + g_sessionMap->size());
	for(auto it = g_sessionMap->begin(); it != g_sessionMap->end(); ++it){
		auto session = it->weakSession.lock();
		if(!session){
			continue;
		}
		ret.emplace(it->loginName, std::move(session));
	}
}

}
