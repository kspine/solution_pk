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
		ItemId item_id;
		boost::uint64_t count;
	};

private:
	const AccountUuid m_account_uuid;

	boost::container::flat_map<ItemId,
		boost::shared_ptr<MySql::Center_Item>> m_items;

public:
	explicit ItemBox(const AccountUuid &account_uuid);
	ItemBox(const AccountUuid &account_uuid,
		const std::vector<boost::shared_ptr<MySql::Center_Item>> &items);
	~ItemBox();

public:
	AccountUuid get_account_uuid() const {
		return m_account_uuid;
	}

	void pump_status(bool force_synchronization_with_client = false);

	ItemInfo get(ItemId item_id) const;
	void get_all(std::vector<ItemInfo> &ret) const;
	ItemId commit_transaction_nothrow(const ItemTransactionElement *elements, std::size_t count,
		const boost::function<void ()> &callback = boost::function<void ()>());
	void commit_transaction(const ItemTransactionElement *elements, std::size_t count,
		const boost::function<void ()> &callback = boost::function<void ()>());
};

}

#endif

