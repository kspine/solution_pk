#ifndef EMPERY_CENTER_FRIEND_RECORD_BOX_HPP_
#define EMPERY_CENTER_FRIEND_RECORD_BOX_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <cstddef>
#include <vector>
#include <deque>
#include <boost/shared_ptr.hpp>
#include "id_types.hpp"
#include "coord.hpp"

namespace EmperyCenter {

namespace MySql {
	class Center_FriendRecord;
}

class FriendRecordBox : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
public:
	enum ResultType {
		RT_FRIEND      = 1,//成为好友
		RT_DECLINE     = 2,//拒绝好友申请(负表示被拒绝)
	};
	struct RecordInfo {
		Poseidon::Uuid auto_uuid;
		std::uint64_t timestamp;
		AccountUuid   friend_account_uuid;
		int    result_type;
	};

private:
	const AccountUuid m_account_uuid;

	std::deque<boost::shared_ptr<MySql::Center_FriendRecord>> m_records;

public:
	FriendRecordBox(AccountUuid account_uuid,
		const std::vector<boost::shared_ptr<MySql::Center_FriendRecord>> &records);
	~FriendRecordBox();

public:
	virtual void pump_status();

	AccountUuid get_account_uuid() const {
		return m_account_uuid;
	}

	void get_all(std::vector<RecordInfo> &ret) const;

	void push(std::uint64_t timestamp,AccountUuid friend_account_uuid,int result_type);
};

}

#endif
