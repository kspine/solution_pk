#ifndef EMPERY_CENTER_DATA_VIP_HPP_
#define EMPERY_CENTER_DATA_VIP_HPP_

#include "common.hpp"

namespace EmperyCenter {

namespace Data {
	class Vip {
	public:
		static boost::shared_ptr<const Vip> get(unsigned vip_level);
		static boost::shared_ptr<const Vip> require(unsigned vip_level);

	public:
		unsigned vip_level;
		double production_turbo;
		unsigned max_castle_count;
		std::uint64_t vip_exp;
		std::uint64_t building_free_update_time;
		std::uint64_t dungeon_count;
		double search_time;
	};
}

}

#endif
