#ifndef EMPERY_PROMOTION_DATA_SNG_LEVEL_HPP_
#define EMPERY_PROMOTION_DATA_SNG_LEVEL_HPP_

#include "common.hpp"

namespace EmperyPromotion {

namespace Data {
	class Promotion {
	public:
		static boost::shared_ptr<const Promotion> get(boost::uint64_t price);
		static boost::shared_ptr<const Promotion> require(boost::uint64_t price);

		static boost::shared_ptr<const Promotion> getNext(boost::uint64_t price);

	public:
		boost::uint64_t price;
		unsigned level;
		std::string name;
		double taxRatio;
		bool taxExtra;
		boost::uint64_t immediatePrice;
		double immediateDiscount;
		unsigned autoUpgradeCount;
	};
}

}

#endif
