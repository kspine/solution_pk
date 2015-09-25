#ifndef EMPERY_PROMOTION_BILL_STATES_HPP_
#define EMPERY_PROMOTION_BILL_STATES_HPP_

namespace EmperyPromotion {

namespace BillStates {
	enum {
		ST_NEW                  = 0,
		ST_CALLBACK_RECEIVED    = 1,
		ST_SETTLED              = 2,

		ST_CANCELLED            = 100,
		ST_ABANDONED            = 101,
	};
}

}

#endif
