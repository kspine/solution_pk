#ifndef EMPERY_CENTER_TRANSACTION_ELEMENT_HPP_
#define EMPERY_CENTER_TRANSACTION_ELEMENT_HPP_

#include <cstdint>

namespace EmperyCenter {

template<typename FriendT, typename SomeIdT, int>
class TransactionElement {
	friend FriendT;

public:
	enum Operation {
		OP_NONE             = 0,
		OP_ADD              = 1,
		OP_REMOVE           = 2,
		OP_REMOVE_SATURATED = 3,
	};

private:
	Operation m_operation;
	SomeIdT m_some_id;
	std::uint64_t m_delta_count;

	ReasonId m_reason;
	std::int64_t m_param1;
	std::int64_t m_param2;
	std::int64_t m_param3;

public:
	TransactionElement(Operation operation, SomeIdT some_id, std::uint64_t delta_count,
		ReasonId reason, std::int64_t param1, std::int64_t param2, std::int64_t param3)
		: m_operation(operation), m_some_id(some_id), m_delta_count(delta_count)
		, m_reason(reason), m_param1(param1), m_param2(param2), m_param3(param3)
	{
	}
};

}

#endif

