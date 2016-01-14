#ifndef EMPERY_CENTER_SINGLETONS_AUCTION_TRANSACTION_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_AUCTION_TRANSACTION_MAP_HPP_

#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include "../id_types.hpp"

namespace EmperyCenter {

class AuctionTransaction;

struct AuctionTransactionMap {
	static boost::shared_ptr<AuctionTransaction> get(const std::string &serial);
	static boost::shared_ptr<AuctionTransaction> require(const std::string &serial);

	static void get_all(std::vector<boost::shared_ptr<AuctionTransaction>> &ret);
	static void get_by_account(std::vector<boost::shared_ptr<AuctionTransaction>> &ret, AccountUuid account_uuid);

	static void insert(const boost::shared_ptr<AuctionTransaction> &auction_transaction);
	static void update(const boost::shared_ptr<AuctionTransaction> &auction_transaction, bool throws_if_not_exists = true);
	static void remove(const std::string &serial) noexcept;

private:
	AuctionTransactionMap() = delete;
};

}

#endif
