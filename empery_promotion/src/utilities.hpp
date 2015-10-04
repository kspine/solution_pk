#ifndef EMPERY_PROMOTION_UTILITIES_HPP_
#define EMPERY_PROMOTION_UTILITIES_HPP_

#include <boost/shared_ptr.hpp>
#include <boost/cstdint.hpp>
#include <string>
#include <utility>
#include "id_types.hpp"

namespace EmperyPromotion {

namespace Data {
	class Promotion;
}

// <succeeded, balanceToConsume>
extern std::pair<bool, boost::uint64_t> tryUpgradeAccount(AccountId accountId, AccountId payerId, bool isCreatingAccount,
	const boost::shared_ptr<const Data::Promotion> &promotionData, const std::string &remarks);

extern void commitFirstBalanceBonus();
extern void accumulateBalanceBonus(AccountId accountId, AccountId payerId, boost::uint64_t amount);

extern std::string generateBillSerial(const std::string &prefix);

}

#endif
