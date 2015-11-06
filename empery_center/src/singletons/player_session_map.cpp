#include "../precompiled.hpp"
#include "player_session_map.hpp"
#include <poseidon/multi_index_map.hpp>
#include <poseidon/cbpp/control_message.hpp>
#include <poseidon/ip_port.hpp>
#include "account_map.hpp"
#include "../msg/kill.hpp"
#include "../events/account.hpp"
#include "../player_session.hpp"

namespace EmperyCenter {

namespace {
	struct SessionElement {
		AccountUuid accountUuid;
		boost::weak_ptr<PlayerSession> weakSession;

		boost::uint64_t onlineSince;

		SessionElement(const AccountUuid &accountUuid_, boost::weak_ptr<PlayerSession> weakSession_)
			: accountUuid(accountUuid_), weakSession(std::move(weakSession_))
			, onlineSince(Poseidon::getFastMonoClock())
		{
		}
		~SessionElement(){
			const auto session = weakSession.lock();
			if(session){
				session->shutdown(Msg::KILL_SHUTDOWN, "The server is being shut down.");
			}
		}
	};

	MULTI_INDEX_MAP(PlayerSessionMapDelegator, SessionElement,
		UNIQUE_MEMBER_INDEX(accountUuid)
		UNIQUE_MEMBER_INDEX(weakSession)
	)

	boost::shared_ptr<PlayerSessionMapDelegator> g_sessionMap;

	MODULE_RAII_PRIORITY(handles, 8000){
		const auto sessionMap = boost::make_shared<PlayerSessionMapDelegator>();
		g_sessionMap = sessionMap;
		handles.push(sessionMap);
	}
}

boost::shared_ptr<PlayerSession> PlayerSessionMap::get(const AccountUuid &accountUuid){
	PROFILE_ME;

	const auto it = g_sessionMap->find<0>(accountUuid);
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

AccountUuid PlayerSessionMap::getAccountUuid(const boost::weak_ptr<PlayerSession> &weakSession){
	PROFILE_ME;

	const auto it = g_sessionMap->find<1>(weakSession);
	if(it == g_sessionMap->end<1>()){
		return { };
	}
	return it->accountUuid;
}
AccountUuid PlayerSessionMap::requireAccountUuid(const boost::weak_ptr<PlayerSession> &weakSession){
	PROFILE_ME;

	auto ret = getAccountUuid(weakSession);
	if(!ret){
		DEBUG_THROW(Exception, sslit("Session not found"));
	}
	return ret;
}

void PlayerSessionMap::add(const AccountUuid &accountUuid, const boost::shared_ptr<PlayerSession> &session){
	PROFILE_ME;

	const auto it = g_sessionMap->find<1>(session);
	if(it != g_sessionMap->end<1>()){
		LOG_EMPERY_CENTER_WARNING("Session added by another account: accountUuid = ", it->accountUuid);
		DEBUG_THROW(Exception, sslit("Duplicate session"));
	}

	const auto localNow = Poseidon::getUtcTime();

	for(;;){
		const auto result = g_sessionMap->insert(SessionElement(accountUuid, session));
		if(result.second){
			// 新会话。
			try {
				AccountMap::setAttribute(accountUuid, AccountMap::ATTR_TIME_LAST_LOGGED_IN,
					boost::lexical_cast<std::string>(localNow));
				AccountMap::setAttribute(accountUuid, AccountMap::ATTR_TIME_LAST_LOGGED_OUT,
					std::string());
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_ERROR("std::exception thrown: accountUuid = ", accountUuid, ", what = ", e.what());
				g_sessionMap->erase(result.first);
				session->forceShutdown();
				throw;
			}

			const auto remoteIp = std::string(session->getRemoteInfo().ip.get());
			LOG_EMPERY_CENTER_INFO("Player goes online: accountUuid = ", accountUuid, ", remoteIp = ", remoteIp);
			Poseidon::asyncRaiseEvent(boost::make_shared<Events::AccountLoggedIn>(accountUuid, remoteIp));
			break;
		}
		const auto otherSession = result.first->weakSession.lock();
		if(otherSession == session){
			// 会话已存在。
			break;
		}
		if(otherSession){
			otherSession->shutdown(Msg::KILL_SESSION_GHOSTED, { });
			LOG_EMPERY_CENTER_INFO("Session ghosted: accountUuid = ", accountUuid);
		}
		g_sessionMap->erase(result.first);
	}
}
void PlayerSessionMap::remove(const boost::weak_ptr<PlayerSession> &weakSession) noexcept {
	PROFILE_ME;

	const auto it = g_sessionMap->find<1>(weakSession);
	if(it == g_sessionMap->end<1>()){
		return;
	}

	const auto session = weakSession.lock();
	if(session){
		session->shutdown(Msg::KILL_OPERATOR_COMMAND, { });
	}

	const auto accountUuid = it->accountUuid;
	const auto onlineDuration = Poseidon::getFastMonoClock() - it->onlineSince;
	g_sessionMap->erase<1>(it);

	const auto localNow = Poseidon::getUtcTime();

	try {
		LOG_EMPERY_CENTER_INFO("Player goes offline: accountUuid = ", accountUuid, ", onlineDuration = ", onlineDuration);
		Poseidon::asyncRaiseEvent(boost::make_shared<Events::AccountLoggedOut>(accountUuid, onlineDuration));

		AccountMap::setAttribute(accountUuid, AccountMap::ATTR_TIME_LAST_LOGGED_OUT,
			boost::lexical_cast<std::string>(localNow));
	} catch(std::exception &e){
		LOG_EMPERY_CENTER_INFO("std::exception thrown: what = ", e.what());
		if(session){
			session->forceShutdown();
		}
	}
}

void PlayerSessionMap::getAll(boost::container::flat_map<AccountUuid, boost::shared_ptr<PlayerSession>> &ret){
	PROFILE_ME;

	ret.reserve(ret.size() + g_sessionMap->size());
	for(auto it = g_sessionMap->begin(); it != g_sessionMap->end(); ++it){
		auto session = it->weakSession.lock();
		if(!session){
			continue;
		}
		ret.emplace(it->accountUuid, std::move(session));
	}
}

}
