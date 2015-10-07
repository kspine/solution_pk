#ifndef EMPERY_PROMOTION_DATA_SNG_LEVEL_HPP_
#define EMPERY_PROMOTION_DATA_SNG_LEVEL_HPP_

#include "common.hpp"

namespace EmperyPromotion {

namespace Data {
	class Promotion {
	public:
		static boost::shared_ptr<const Promotion> get(boost::uint64_t level);
		static boost::shared_ptr<const Promotion> require(boost::uint64_t level);

		static boost::shared_ptr<const Promotion> getNext(boost::uint64_t level);

	public:
		boost::uint64_t level;
		unsigned displayLevel;
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
