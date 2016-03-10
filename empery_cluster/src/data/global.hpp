#ifndef EMPERY_CLUSTER_DATA_GLOBAL_HPP_
#define EMPERY_CLUSTER_DATA_GLOBAL_HPP_

#include "common.hpp"
#include <poseidon/fwd.hpp>

namespace EmperyCluster {

namespace Data {
	class Global {
	public:
		enum Slot {
			SLOT_MAP_BORDER_THICKNESS                               = 100025,
			SLOT_MAP_MONSTER_ACTIVE_SCOPE                           = 100050,
		};

	public:
		static const std::string &as_string(Slot slot);
		static std::int64_t as_signed(Slot slot);
		static std::uint64_t as_unsigned(Slot slot);
		static double as_double(Slot slot);
		static const Poseidon::JsonArray &as_array(Slot slot);
		static const Poseidon::JsonObject &as_object(Slot slot);
	};
}

}

#endif
