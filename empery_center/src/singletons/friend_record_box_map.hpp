#ifndef EMPERY_CENTER_SINGLETONS_FRIEND_RECORD_BOX_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_FRIEND_RECORD_BOX_MAP_HPP_

#include <boost/shared_ptr.hpp>
#include "../id_types.hpp"

namespace EmperyCenter {

class FriendRecordBox;

struct FriendRecordBoxMap {
	static void insert(AccountUuid account_uuid,AccountUuid friend_uuid,std::uint64_t utc_now,int result_type);
	static boost::shared_ptr<FriendRecordBox> get(AccountUuid account_uuid);
	static boost::shared_ptr<FriendRecordBox> require(AccountUuid account_uuid);
	static void unload(AccountUuid account_uuid);

private:
	FriendRecordBoxMap() = delete;
};

}

#endif
