#ifndef EMPERY_PROMOTION_SINGLETONS_ITEM_MAP_HPP_
#define EMPERY_PROMOTION_SINGLETONS_ITEM_MAP_HPP_

#include <cstddef>
#include <boost/container/flat_map.hpp>
#include <boost/cstdint.hpp>
#include "../id_types.hpp"

namespace EmperyPromotion {

class ItemTransactionElement;

struct ItemMap {
	static boost::uint64_t getCount(AccountId accountId, ItemId itemId);
	static void getAllByAccountId(boost::container::flat_map<ItemId, boost::uint64_t> &ret, AccountId accountId);
	static void getAllByItemId(boost::container::flat_map<AccountId, boost::uint64_t> &ret, ItemId itemId);

	static void commitTransaction(const ItemTransactionElement *elements, std::size_t count);
	// 返回不足的道具数量。
	static ItemId commitTransactionNoThrow(const ItemTransactionElement *elements, std::size_t count);

private:
	ItemMap() = delete;
};

}

#endif
