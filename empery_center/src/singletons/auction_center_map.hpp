#ifndef EMPERY_CENTER_SINGLETONS_AUCTION_CENTER_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_AUCTION_CENTER_MAP_HPP_

#include <boost/shared_ptr.hpp>
#include "../id_types.hpp"

namespace EmperyCenter {

class AuctionCenter;

struct AuctionCenterMap {
	static boost::shared_ptr<AuctionCenter> get(AccountUuid account_uuid);
	static boost::shared_ptr<AuctionCenter> require(AccountUuid account_uuid);

private:
	AuctionCenterMap() = delete;
};

}

#endif
