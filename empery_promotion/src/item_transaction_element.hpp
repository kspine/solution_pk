#ifndef EMPERY_PROMOTION_ITEM_TRANSACTION_ELEMENT_HPP_
#define EMPERY_PROMOTION_ITEM_TRANSACTION_ELEMENT_HPP_

#include <cstdint>
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
	AccountId m_account_id;
	Operation m_operation;
	ItemId m_item_id;
	std::uint64_t m_delta_count;

	Events::ItemChanged::Reason m_reason;
	std::uint64_t m_param1;
	std::uint64_t m_param2;
	std::uint64_t m_param3;
	std::string m_remarks;

public:
	ItemTransactionElement()
		: m_account_id(0), m_operation(OP_NONE), m_item_id(0), m_delta_count(0)
	{
	}
	ItemTransactionElement(AccountId account_id, Operation operation, ItemId item_id, std::uint64_t delta_count,
		Events::ItemChanged::Reason reason, std::uint64_t param1, boost::uint64_t param2, boost::uint64_t param3, std::string remarks)
		: m_account_id(account_id), m_operation(operation), m_item_id(item_id), m_delta_count(delta_count)
		, m_reason(reason), m_param1(param1), m_param2(param2), m_param3(param3), m_remarks(std::move(remarks))
	{
	}
};

}

#endif
