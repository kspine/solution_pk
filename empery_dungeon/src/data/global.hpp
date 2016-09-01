#ifndef EMPERY_DUNGEON_DATA_GLOBAL_HPP_
#define EMPERY_DUNGEON_DATA_GLOBAL_HPP_

#include "common.hpp"
#include <poseidon/fwd.hpp>

namespace EmperyDungeon {

namespace Data {
	class Global {
	public:
		enum Slot {
			SLOT_DUNGEON_MONSTER_CREATE_RANGE                              = 100025,
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
