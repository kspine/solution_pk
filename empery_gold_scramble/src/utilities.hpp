#ifndef EMPERY_GOLD_SCRAMBLE_UTILITIES_HPP_
#define EMPERY_GOLD_SCRAMBLE_UTILITIES_HPP_

#include <boost/cstdint.hpp>

namespace EmperyGoldScramble {

class PlayerSession;

extern void sendAuctionStatusToClient(const boost::shared_ptr<PlayerSession> &session);
extern void updateAuctionStatus();

}

#endif
