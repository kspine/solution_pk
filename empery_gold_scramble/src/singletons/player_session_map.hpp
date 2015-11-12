#ifndef EMPERY_GOLD_SCRAMBLE_SINGLETONS_PLAYER_SESSION_MAP_HPP_
#define EMPERY_GOLD_SCRAMBLE_SINGLETONS_PLAYER_SESSION_MAP_HPP_

#include <boost/container/flat_map.hpp>
#include <string>
#include <boost/shared_ptr.hpp>
#include <poseidon/cbpp/fwd.hpp>

namespace EmperyGoldScramble {

class PlayerSession;

struct PlayerSessionMap {
	static boost::shared_ptr<PlayerSession> get(const std::string &loginName);

	static std::string getLoginName(const boost::weak_ptr<PlayerSession> &weakSession);
	static std::string requireLoginName(const boost::weak_ptr<PlayerSession> &weakSession);

	static std::string getNick(const boost::weak_ptr<PlayerSession> &weakSession);
	static std::string requireNick(const boost::weak_ptr<PlayerSession> &weakSession);

	static void add(const std::string &loginName, const std::string &nick, const boost::shared_ptr<PlayerSession> &session);
	static void remove(const boost::weak_ptr<PlayerSession> &weakSession) noexcept;

	static void getAll(boost::container::flat_map<std::string, boost::shared_ptr<PlayerSession>> &ret);

private:
	PlayerSessionMap() = delete;
};

}

#endif
