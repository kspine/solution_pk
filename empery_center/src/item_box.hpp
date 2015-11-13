#ifndef EMPERY_CENTER_ITEM_BOX_HPP_
#define EMPERY_CENTER_ITEM_BOX_HPP_

#include <cstddef>
#include <vector>
#include <boost/container/flat_map.hpp>
#include <boost/shared_ptr.hpp>
#include <poseidon/cxx_util.hpp>
#include <boost/function.hpp>
#include "id_types.hpp"

namespace EmperyCenter {

namespace MySql {
	class Center_Item;
}

class ItemTransactionElement;

class ItemBox : NONCOPYABLE {
public:
	struct ItemInfo {
		ItemId itemId;
		boost::uint64_t count;
	};

private:
	const AccountUuid m_accountUuid;

	boost::container::flat_map<ItemId,
		boost::shared_ptr<MySql::Center_Item>> m_items;

public:
	explicit ItemBox(const AccountUuid &accountUuid);
	ItemBox(const AccountUuid &accountUuid,
		const std::vector<boost::shared_ptr<MySql::Center_Item>> &items);
	~ItemBox();

public:
	AccountUuid getAccountUuid() const {
		return m_accountUuid;
	}

	void pumpStatus(bool forceSynchronizationWithClient = false);

	ItemInfo get(ItemId itemId) const;
	void getAll(std::vector<ItemInfo> &ret) const;
	ItemId commitTransactionNoThrow(const ItemTransactionElement *elements, std::size_t count,
		const boost::function<void ()> &callback = boost::function<void ()>());
	void commitTransaction(const ItemTransactionElement *elements, std::size_t count,
		const boost::function<void ()> &callback = boost::function<void ()>());
};

}

#endif

