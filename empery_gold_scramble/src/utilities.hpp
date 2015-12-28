#ifndef EMPERY_GOLD_SCRAMBLE_UTILITIES_HPP_
#define EMPERY_GOLD_SCRAMBLE_UTILITIES_HPP_

#include <cstdint>

namespace EmperyGoldScramble {

class PlayerSession;

extern void send_auction_status_to_client(const boost::shared_ptr<PlayerSession> &session);
extern void send_last_log_to_client(const boost::shared_ptr<PlayerSession> &session);
extern void invalidate_auction_status();

}

#endif
