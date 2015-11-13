#ifndef EMPERY_CENTER_ITEM_TRANSACTION_ELEMENT_HPP_
#define EMPERY_CENTER_ITEM_TRANSACTION_ELEMENT_HPP_

#include <boost/cstdint.hpp>
#include "id_types.hpp"

namespace EmperyCenter {

class ItemBox;

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

}

#endif

