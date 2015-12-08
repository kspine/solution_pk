#include "../precompiled.hpp"
#include "player_session_map.hpp"
#include <poseidon/multi_index_map.hpp>
#include <poseidon/cbpp/control_message.hpp>
#include <poseidon/ip_port.hpp>
#include "../account.hpp"
#include "../account_attribute_ids.hpp"
#include "../msg/kill.hpp"
#include "../events/account.hpp"
#include "../player_session.hpp"

namespace EmperyCenter {

namespace {
	struct SessionElement {
		boost::shared_ptr<Account> account;
		boost::uint64_t online_since;

		AccountUuid account_uuid;
		boost::weak_ptr<PlayerSession> weak_session;

		SessionElement(boost::shared_ptr<Account> account_, boost::weak_ptr<PlayerSession> weak_session_)
			: account(std::move(account_)), online_since(Poseidon::get_fast_mono_clock())
			, account_uuid(account->get_account_uuid()), weak_session(std::move(weak_session_))
		{
		}
	};

	MULTI_INDEX_MAP(PlayerSessionMapContainer, SessionElement,
		UNIQUE_MEMBER_INDEX(account_uuid)
		UNIQUE_MEMBER_INDEX(weak_session)
	)

	boost::shared_ptr<PlayerSessionMapContainer> g_session_map;

	class SessionMapGuard {
	private:
		boost::shared_ptr<PlayerSessionMapContainer> m_session_map;

	public:
		explicit SessionMapGuard(boost::shared_ptr<PlayerSessionMapContainer> session_map)
			: m_session_map(std::move(session_map))
		{
		}
		~SessionMapGuard(){
			for(auto it = m_session_map->begin(); it != m_session_map->end(); ++it){
				const auto session = it->weak_session.lock();
				if(!session){
					continue;
				}
				session->shutdown(Msg::KILL_SHUTDOWN, "The server is being shut down");
			}
		}
	};

	MODULE_RAII_PRIORITY(handles, 8000){
		const auto session_map = boost::make_shared<PlayerSessionMapContainer>();
		g_session_map = session_map;
		handles.push(session_map);
		handles.push(boost::make_shared<SessionMapGuard>(session_map));
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
AccountUuid PlayerSessionMap::require_account_uuid(const boost::weak_ptr<PlayerSession> &weak_session){
	PROFILE_ME;

	auto ret = get_account_uuid(weak_session);
	if(!ret){
		DEBUG_THROW(Exception, sslit("Session not found"));
	}
	return ret;
}

void PlayerSessionMap::add(const boost::shared_ptr<Account> &account, const boost::shared_ptr<PlayerSession> &session){
	PROFILE_ME;

	const auto it = g_session_map->find<1>(session);
	if(it != g_session_map->end<1>()){
		LOG_EMPERY_CENTER_WARNING("Session added by another account: account_uuid = ", it->account_uuid);
		DEBUG_THROW(Exception, sslit("Duplicate session"));
	}

	const auto utc_now = Poseidon::get_utc_time();

	const auto account_uuid = account->get_account_uuid();

	for(;;){
		const auto result = g_session_map->insert(SessionElement(account, session));
		if(result.second){
			// 新会话。
			try {
				boost::container::flat_map<AccountAttributeId, std::string> modifiers;
				modifiers.reserve(2);
				modifiers[AccountAttributeIds::ID_LAST_LOGGED_IN_TIME]  = boost::lexical_cast<std::string>(utc_now);
				modifiers[AccountAttributeIds::ID_LAST_LOGGED_OUT_TIME] = { };
				account->set_attributes(std::move(modifiers));
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_ERROR("std::exception thrown: account_uuid = ", account_uuid, ", what = ", e.what());
				g_session_map->erase(result.first);
				session->shutdown(e.what());
				throw;
			}

			const auto remote_ip = std::string(session->get_remote_info().ip.get());
			LOG_EMPERY_CENTER_INFO("Player goes online: account_uuid = ", account_uuid, ", remote_ip = ", remote_ip);
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
			LOG_EMPERY_CENTER_INFO("Session ghosted: account_uuid = ", account_uuid);
		}
		g_session_map->erase(result.first);
	}
}
void PlayerSessionMap::remove(const boost::weak_ptr<PlayerSession> &weak_session) noexcept {
	PROFILE_ME;

	const auto it = g_session_map->find<1>(weak_session);
	if(it == g_session_map->end<1>()){
		return;
	}

	const auto session = weak_session.lock();
	if(session){
		session->shutdown(Msg::KILL_OPERATOR_COMMAND, { });
	}

	const auto now     = Poseidon::get_fast_mono_clock();
	const auto utc_now = Poseidon::get_utc_time();

	const auto account         = it->account;
	const auto account_uuid    = account->get_account_uuid();
	const auto online_duration = now - it->online_since;
	g_session_map->erase<1>(it);

	try {
		LOG_EMPERY_CENTER_INFO("Player goes offline: account_uuid = ", account_uuid, ", online_duration = ", online_duration);
		Poseidon::async_raise_event(boost::make_shared<Events::AccountLoggedOut>(account_uuid, online_duration));

		boost::container::flat_map<AccountAttributeId, std::string> modifiers;
		modifiers[AccountAttributeIds::ID_LAST_LOGGED_OUT_TIME] = boost::lexical_cast<std::string>(utc_now);
		account->set_attributes(std::move(modifiers));
	} catch(std::exception &e){
		LOG_EMPERY_CENTER_INFO("std::exception thrown: what = ", e.what());
		if(session){
			session->shutdown(e.what());
		}
	}
}

void PlayerSessionMap::get_all(boost::container::flat_map<AccountUuid, boost::shared_ptr<PlayerSession>> &ret){
	PROFILE_ME;

	ret.reserve(ret.size() + g_session_map->size());
	for(auto it = g_session_map->begin(); it != g_session_map->end(); ++it){
		auto session = it->weak_session.lock();
		if(!session){
			continue;
		}
		ret.emplace(it->account_uuid, std::move(session));
	}
}
void PlayerSessionMap::clear(const char *reason) noexcept {
	PROFILE_ME;

	for(auto it = g_session_map->begin(); it != g_session_map->end(); ++it){
		auto session = it->weak_session.lock();
		if(!session){
			continue;
		}
		session->shutdown(reason);
	}
	g_session_map->clear();
}
void PlayerSessionMap::clear(int code, const char *reason) noexcept {
	PROFILE_ME;

	for(auto it = g_session_map->begin(); it != g_session_map->end(); ++it){
		auto session = it->weak_session.lock();
		if(!session){
			continue;
		}
		session->shutdown(code, reason);
	}
	g_session_map->clear();
}

}
