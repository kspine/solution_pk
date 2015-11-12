#ifndef EMPERY_CENTER_SINGLETONS_PLAYER_SESSION_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_PLAYER_SESSION_MAP_HPP_

#include <boost/container/flat_map.hpp>
#include <utility>
#include <boost/shared_ptr.hpp>
#include <poseidon/cxx_ver.hpp>
#include <poseidon/cbpp/fwd.hpp>
#include "../id_types.hpp"

namespace EmperyCenter {

class PlayerSession;

struct PlayerSessionMap {
	static boost::shared_ptr<PlayerSession> get(const AccountUuid &accountUuid);

	static AccountUuid getAccountUuid(const boost::weak_ptr<PlayerSession> &weakSession);
	static AccountUuid requireAccountUuid(const boost::weak_ptr<PlayerSession> &weakSession);

	static void add(const AccountUuid &accountUuid, const boost::shared_ptr<PlayerSession> &session);
	static void remove(const boost::weak_ptr<PlayerSession> &weakSession) noexcept;
	static void threadSafeRemove(const boost::weak_ptr<PlayerSession> &weakSession) noexcept;

	static void getAll(boost::container::flat_map<AccountUuid, boost::shared_ptr<PlayerSession>> &ret);

private:
	PlayerSessionMap() = delete;
};

}

#endif
