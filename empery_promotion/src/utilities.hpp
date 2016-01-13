#ifndef EMPERY_PROMOTION_UTILITIES_HPP_
#define EMPERY_PROMOTION_UTILITIES_HPP_

#include <boost/shared_ptr.hpp>
#include <cstdint>
#include <string>
#include <utility>
#include "id_types.hpp"

namespace EmperyPromotion {

namespace Data {
	class Promotion;
}

// <succeeded, balance_to_consume>
extern std::pair<bool, std::uint64_t> try_upgrade_account(AccountId account_id, AccountId payer_id, bool is_creating_account,
	const boost::shared_ptr<const Data::Promotion> &promotion_data, const std::string &remarks, std::uint64_t additional_cards);
extern void check_auto_upgradeable(AccountId init_account_id);

extern void commit_first_balance_bonus();
extern void accumulate_balance_bonus(AccountId account_id, AccountId payer_id, std::uint64_t amount, std::uint64_t upgrade_to_level);
extern void accumulate_balance_bonus_abs(AccountId account_id, std::uint64_t amount);

extern std::uint64_t sell_acceleration_cards(AccountId buyer_id, std::uint64_t unit_price, std::uint64_t cards_to_sell);

extern std::string generate_bill_serial(const std::string &prefix);

}

#endif
