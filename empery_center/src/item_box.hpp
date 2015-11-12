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

class ItemBox : NONCOPYABLE {
public:
	struct ItemInfo {
		ItemId itemId;
		boost::uint64_t count;
	};

	class ItemTransactionElement {
		friend ItemBox;

	public:
		enum Operation {
			OP_NONE             = 0,
			OP_ADD              = 1,
			OP_REMOVE           = 2,
			OP_REMOVE_SATURATED = 3,
		};

	private:
		Operation m_operation;
		ItemId m_itemId;
		boost::uint64_t m_deltaCount;

		ReasonId m_reason;
		boost::uint64_t m_param1;
		boost::uint64_t m_param2;
		boost::uint64_t m_param3;

	public:
		ItemTransactionElement(Operation operation, ItemId itemId, boost::uint64_t deltaCount,
			ReasonId reason, boost::uint64_t param1, boost::uint64_t param2, boost::uint64_t param3)
			: m_operation(operation), m_itemId(itemId), m_deltaCount(deltaCount)
			, m_reason(reason), m_param1(param1), m_param2(param2), m_param3(param3)
		{
		}
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

