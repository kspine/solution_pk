#include "../precompiled.hpp"
#include "player_session_map.hpp"
#include <poseidon/multi_index_map.hpp>
#include <poseidon/cbpp/control_message.hpp>
#include <poseidon/ip_port.hpp>
#include "account_map.hpp"
#include "../msg/kill.hpp"
#include "../player_session.hpp"

namespace EmperyCenter {

namespace {
	struct SessionElement {
		AccountUuid accountUuid;
		boost::weak_ptr<PlayerSession> session;

		boost::uint64_t onlineSince;

		SessionElement(const AccountUuid &accountUuid_, boost::weak_ptr<PlayerSession> session_)
			: accountUuid(accountUuid_), session(std::move(session_))
			, onlineSince(Poseidon::getFastMonoClock())
		{
		}
	};

	MULTI_INDEX_MAP(PlayerSessionMapDelegator, SessionElement,
		UNIQUE_MEMBER_INDEX(accountUuid)
		UNIQUE_MEMBER_INDEX(session)
	)

	boost::shared_ptr<PlayerSessionMapDelegator> g_sessionMap;

	MODULE_RAII_PRIORITY(handles, 8000){
		const auto sessionMap = boost::make_shared<PlayerSessionMapDelegator>();
		g_sessionMap = sessionMap;
		handles.push(sessionMap);
	}
}
/*
boost::shared_ptr<PlayerSession> PlayerSessionMap::get(const AccountUuid &accountUuid);

AccountUuid PlayerSessionMap::getAccountUuid(const boost::weak_ptr<PlayerSession> &weakSession);
AccountUuid PlayerSessionMap::requireAccountUuid(const boost::weak_ptr<PlayerSession> &weakSession);

void PlayerSessionMap::add(const AccountUuid &accountUuid, const boost::shared_ptr<PlayerSession> &session);
void PlayerSessionMap::remove(const boost::weak_ptr<PlayerSession> &weakSession) noexcept;

void PlayerSessionMap::getAll(boost::container::flat_map<AccountUuid, boost::weak_ptr<PlayerSession>> &ret);
*/
}
