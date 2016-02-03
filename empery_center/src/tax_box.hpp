#ifndef EMPERY_CENTER_TAX_BOX_HPP_
#define EMPERY_CENTER_TAX_BOX_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <cstddef>
#include <vector>
#include <deque>
#include <boost/shared_ptr.hpp>
#include "id_types.hpp"

namespace EmperyCenter {

namespace MySql {
	class Center_TaxRecord;
}

class TaxBox : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
public:
	struct RecordInfo {
		std::uint64_t auto_inc;
		std::uint64_t timestamp;
		AccountUuid from_account_uuid;
		ReasonId reason;
		std::uint64_t old_amount;
		std::uint64_t new_amount;
	};

private:
	const AccountUuid m_account_uuid;

	std::deque<boost::shared_ptr<MySql::Center_TaxRecord>> m_records;
	std::uint64_t m_auto_inc = 0;

public:
	TaxBox(AccountUuid account_uuid,
		const std::vector<boost::shared_ptr<MySql::Center_TaxRecord>> &records);
	~TaxBox();

public:
	virtual void pump_status();

	AccountUuid get_account_uuid() const {
		return m_account_uuid;
	}

	void get_all(std::vector<RecordInfo> &ret) const;

	void push(std::uint64_t timestamp, AccountUuid from_account_uuid,
		ReasonId reason, std::uint64_t old_amount, std::uint64_t new_amount);
};

}

#endif
