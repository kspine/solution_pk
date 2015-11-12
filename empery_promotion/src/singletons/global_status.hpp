#ifndef EMPERY_PROMOTION_SINGLETONS_GLOBAL_STATUS_HPP_
#define EMPERY_PROMOTION_SINGLETONS_GLOBAL_STATUS_HPP_

#include <cstddef>
#include <boost/cstdint.hpp>

namespace EmperyPromotion {

struct GlobalStatus {
	enum {
		SLOT_SERVER_CREATED_TIME        =    0,
		SLOT_FIRST_BALANCING_TIME       =    1,
		SLOT_ACC_CARD_UNIT_PRICE        =    2,

		SLOT_SERVER_DAILY_RESET_TIME    = 1000,
		SLOT_BILL_AUTO_INC              = 1001,
		SLOT_FIRST_BALANCING_DONE       = 1002,
	};

	static boost::uint64_t get(unsigned slot);
	static boost::uint64_t set(unsigned slot, boost::uint64_t newValue);
	static boost::uint64_t fetchAdd(unsigned slot, boost::uint64_t deltaValue);

	static void checkDailyReset();

private:
	GlobalStatus() = delete;
};

}

#endif
