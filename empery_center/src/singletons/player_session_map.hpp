#ifndef EMPERY_CENTER_SINGLETONS_PLAYER_SESSION_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_PLAYER_SESSION_MAP_HPP_

#include <utility>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <poseidon/cxx_ver.hpp>
#include <poseidon/cbpp/fwd.hpp>
#include "../id_types.hpp"

namespace EmperyCenter {

class Account;
class PlayerSession;

struct PlayerSessionMap {
	static boost::shared_ptr<PlayerSession> get(AccountUuid account_uuid);

	static boost::shared_ptr<Account> get_account(const boost::weak_ptr<PlayerSession> &weak_session);
	static boost::shared_ptr<Account> require_account(const boost::weak_ptr<PlayerSession> &weak_session);

	static void add(const boost::shared_ptr<Account> &account, const boost::shared_ptr<PlayerSession> &session);
	static void remove(const boost::weak_ptr<PlayerSession> &weak_session) noexcept;
	static void async_begin_gc() noexcept;

	static void get_all(std::vector<std::pair<boost::shared_ptr<Account>, boost::shared_ptr<PlayerSession>>> &ret);
	static void clear(const char *reason) noexcept;
	static void clear(int code, const char *reason) noexcept;

private:
	PlayerSessionMap() = delete;
};

}

#endif
