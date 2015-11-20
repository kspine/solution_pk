#ifndef EMPERY_CENTER_DATA_SIGNING_IN_HPP_
#define EMPERY_CENTER_DATA_SIGNING_IN_HPP_

#include "common.hpp"

namespace EmperyCenter {

namespace Data {
	class SigningIn {
	public:
		static boost::shared_ptr<const SigningIn> get(unsigned sequential_days);
		static boost::shared_ptr<const SigningIn> require(unsigned sequential_days);

		static unsigned get_max_sequential_days();

	public:
		unsigned sequential_days;
		TradeId trade_id;
	};
}

}

#endif
