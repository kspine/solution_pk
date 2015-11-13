#ifndef EMPERY_PROMOTION_SINGLETONS_ITEM_MAP_HPP_
#define EMPERY_PROMOTION_SINGLETONS_ITEM_MAP_HPP_

#include <cstddef>
#include <boost/container/flat_map.hpp>
#include <boost/cstdint.hpp>
#include "../id_types.hpp"

namespace EmperyPromotion {

class ItemTransactionElement;

struct ItemMap {
	static boost::uint64_t get_count(AccountId account_id, ItemId item_id);
	static void get_all_by_account_id(boost::container::flat_map<ItemId, boost::uint64_t> &ret, AccountId account_id);
	static void get_all_by_item_id(boost::container::flat_map<AccountId, boost::uint64_t> &ret, ItemId item_id);

	static void commit_transaction(const ItemTransactionElement *elements, std::size_t count);
	// 返回不足的道具数量。
	static ItemId commit_transaction_nothrow(const ItemTransactionElement *elements, std::size_t count);

private:
	ItemMap() = delete;
};

}

#endif
