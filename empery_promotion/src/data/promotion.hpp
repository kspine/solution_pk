#ifndef EMPERY_PROMOTION_DATA_SNG_LEVEL_HPP_
#define EMPERY_PROMOTION_DATA_SNG_LEVEL_HPP_

#include "common.hpp"
#include <vector>

namespace EmperyPromotion {

namespace Data {
	class Promotion {
	public:
		static boost::shared_ptr<const Promotion> get(std::uint64_t level);
		static boost::shared_ptr<const Promotion> require(std::uint64_t level);

		static boost::shared_ptr<const Promotion> get_by_display_level(unsigned display_level);
		static boost::shared_ptr<const Promotion> require_by_display_level(unsigned display_level);

		static void get_all(std::vector<boost::shared_ptr<const Promotion>> &ret);

		static boost::shared_ptr<const Promotion> get_first();
		static boost::shared_ptr<const Promotion> get_next(std::uint64_t level);

	public:
		std::uint64_t level;
		unsigned display_level;
		std::string name;
		double tax_ratio;
		bool tax_extra;
		std::uint64_t immediate_price;
		double immediate_discount;
		unsigned auto_upgrade_count;
		std::uint64_t large_gift_box_price;
		bool shared_recharge_enabled;
	};
}

}

#endif
