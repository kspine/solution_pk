#ifndef EMPERY_PROMOTION_SINGLETONS_SHARED_RECHARGE_MAP_HPP_
#define EMPERY_PROMOTION_SINGLETONS_SHARED_RECHARGE_MAP_HPP_

#include <cstddef>
#include <cstdint>
#include <vector>
#include "../id_types.hpp"

namespace EmperyPromotion {

struct SharedRechargeMap {
	enum State {
		ST_DELETED      = 0,
		ST_REQUESTING   = 1,
		ST_FORMED       = 2,
	};

	struct SharedRechargeInfo {
		AccountId account_id;
		AccountId recharge_to_account_id;
		std::uint64_t updated_time;
		State state;
		std::uint64_t amount;
	};

	static SharedRechargeInfo get(AccountId account_id, AccountId recharge_to_account_id);
	static void get_all(std::vector<SharedRechargeInfo> &ret);
	static void get_by_account(std::vector<SharedRechargeInfo> &ret, AccountId account_id);
	static void get_by_recharge_to_account(std::vector<SharedRechargeInfo> &ret, AccountId recharge_to_account_id);

	static void add(AccountId account_id, AccountId recharge_to_account_id, std::uint64_t amount);
	static void commit(AccountId account_id, AccountId recharge_to_account_id);
	static void rollback(AccountId account_id, AccountId recharge_to_account_id);

private:
	SharedRechargeMap() = delete;
};

}

#endif
