#ifndef EMPERY_PROMOTION_ITEM_TRANSACTION_ELEMENT_HPP_
#define EMPERY_PROMOTION_ITEM_TRANSACTION_ELEMENT_HPP_

#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include "id_types.hpp"

namespace EmperyPromotion {

class ItemMap;

class ItemTransactionElement {
	friend ItemMap;

public:
	enum Operation {
		OP_NONE				= 0,
		OP_ADD				= 1,
		OP_REMOVE			= 2,
		OP_REMOVE_SATURATED	= 3,
	};

public:
	typedef boost::function<void ()> Callback;

private:
	AccountId m_accountId;
	Operation m_operation;
	ItemId m_itemId;
	boost::uint64_t m_deltaCount;

	Callback m_callback; // 抛出的任何异常都会被直接吃掉。

public:
	ItemTransactionElement()
		: m_accountId(0), m_operation(OP_NONE), m_itemId(0), m_deltaCount(0)
	{
	}
	ItemTransactionElement(AccountId accountId, Operation operation, ItemId itemId, boost::uint64_t deltaCount,
		Callback callback = Callback())
		: m_accountId(accountId), m_operation(operation), m_itemId(itemId), m_deltaCount(deltaCount)
		, m_callback(std::move(callback))
	{
	}
};

}

#endif
