#ifndef EMPERY_PROMOTION_DATA_SNG_LEVEL_HPP_
#define EMPERY_PROMOTION_DATA_SNG_LEVEL_HPP_

#include "common.hpp"

namespace EmperyPromotion {

namespace Data {
	class Promotion {
	public:
		static boost::shared_ptr<const Promotion> get(boost::uint64_t level);
		static boost::shared_ptr<const Promotion> require(boost::uint64_t level);

		static boost::shared_ptr<const Promotion> get_first();
		static boost::shared_ptr<const Promotion> get_next(boost::uint64_t level);

	public:
		boost::uint64_t level;
		unsigned display_level;
		std::string name;
		double tax_ratio;
		bool tax_extra;
		boost::uint64_t immediate_price;
		double immediate_discount;
		unsigned auto_upgrade_count;
	};
}

}

#endif
