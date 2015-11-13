#ifndef EMPERY_GOLD_SCRAMBLE_SINGLETONS_PLAYER_SESSION_MAP_HPP_
#define EMPERY_GOLD_SCRAMBLE_SINGLETONS_PLAYER_SESSION_MAP_HPP_

#include <boost/container/flat_map.hpp>
#include <string>
#include <boost/shared_ptr.hpp>
#include <poseidon/cbpp/fwd.hpp>

namespace EmperyGoldScramble {

class PlayerSession;

struct PlayerSessionMap {
	static boost::shared_ptr<PlayerSession> get(const std::string &login_name);

	static std::string get_login_name(const boost::weak_ptr<PlayerSession> &weak_session);
	static std::string require_login_name(const boost::weak_ptr<PlayerSession> &weak_session);

	static std::string get_nick(const boost::weak_ptr<PlayerSession> &weak_session);
	static std::string require_nick(const boost::weak_ptr<PlayerSession> &weak_session);

	static void add(const std::string &login_name, const std::string &nick, const boost::shared_ptr<PlayerSession> &session);
	static void remove(const boost::weak_ptr<PlayerSession> &weak_session) noexcept;

	static void get_all(boost::container::flat_map<std::string, boost::shared_ptr<PlayerSession>> &ret);

private:
	PlayerSessionMap() = delete;
};

}

#endif
