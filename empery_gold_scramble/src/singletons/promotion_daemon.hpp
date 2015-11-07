#ifndef EMPERY_GOLD_SCRAMBLE_SINGLETONS_PROMOTION_DAEMON_HPP_
#define EMPERY_GOLD_SCRAMBLE_SINGLETONS_PROMOTION_DAEMON_HPP_

#include <boost/shared_ptr.hpp>
#include <poseidon/cbpp/fwd.hpp>
#include <poseidon/optional_map.hpp>

namespace EmperyGoldScramble {

struct PromotionDaemon {
	static std::string getLoginNameFromSessionId(const std::string &sessionId);

private:
	PromotionDaemon();
};

}

#endif
