#ifndef EMPERY_CENTER_SINGLETONS_PAYMENT_TRANSACTION_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_PAYMENT_TRANSACTION_MAP_HPP_

#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include "../id_types.hpp"

namespace EmperyCenter {

class PaymentTransaction;

struct PaymentTransactionMap {
	static boost::shared_ptr<PaymentTransaction> get(const std::string &serial);
	static boost::shared_ptr<PaymentTransaction> require(const std::string &serial);

	static void get_all(std::vector<boost::shared_ptr<PaymentTransaction>> &ret);
	static void get_by_account(std::vector<boost::shared_ptr<PaymentTransaction>> &ret, AccountUuid account_uuid);

	static void insert(const boost::shared_ptr<PaymentTransaction> &payment_transaction);
	static void update(const boost::shared_ptr<PaymentTransaction> &payment_transaction, bool throws_if_not_exists = true);
	static void remove(const std::string &serial) noexcept;

private:
	PaymentTransactionMap() = delete;
};

}

#endif
