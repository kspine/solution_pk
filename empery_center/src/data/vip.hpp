#ifndef EMPERY_CENTER_DATA_VIP_HPP_
#define EMPERY_CENTER_DATA_VIP_HPP_

#include "common.hpp"

namespace EmperyCenter {

namespace Data {
	class Vip {
	public:
		static boost::shared_ptr<const Vip> get(unsigned sequential_days);
		static boost::shared_ptr<const Vip> require(unsigned sequential_days);

	public:
		unsigned vip_level;
		double production_turbo;
		unsigned max_castle_count;
	};
}

}

#endif
