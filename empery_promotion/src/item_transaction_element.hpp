#ifndef EMPERY_PROMOTION_ITEM_TRANSACTION_ELEMENT_HPP_
#define EMPERY_PROMOTION_ITEM_TRANSACTION_ELEMENT_HPP_

#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>
#include "id_types.hpp"
#include "events/item.hpp"

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

private:
	AccountId m_accountId;
	Operation m_operation;
	ItemId m_itemId;
	boost::uint64_t m_deltaCount;

	Events::ItemChanged::Reason m_reason;
	boost::uint64_t m_param1;
	boost::uint64_t m_param2;
	boost::uint64_t m_param3;
	std::string m_remarks;

public:
	ItemTransactionElement()
		: m_accountId(0), m_operation(OP_NONE), m_itemId(0), m_deltaCount(0)
	{
	}
	ItemTransactionElement(AccountId accountId, Operation operation, ItemId itemId, boost::uint64_t deltaCount,
		Events::ItemChanged::Reason reason, boost::uint64_t param1, boost::uint64_t param2, boost::uint64_t param3, std::string remarks)
		: m_accountId(accountId), m_operation(operation), m_itemId(itemId), m_deltaCount(deltaCount)
		, m_reason(reason), m_param1(param1), m_param2(param2), m_param3(param3), m_remarks(std::move(remarks))
	{
	}
};

}

#endif
